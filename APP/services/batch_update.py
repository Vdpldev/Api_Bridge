import asyncio
import logging
from datetime import datetime, timezone
from typing import Optional
from sqlalchemy.ext.asyncio import AsyncSession

from APP.db.sessions import AsyncSessionLocal
from APP.crud import batch as batch_crud
from APP.crud import device as device_crud
from APP.crud import logs as logs_crud
from APP.models.batch import BatchUpdateState
from APP.schemas.logs import AdminLogCreate, LogLevel

# Configure logging
logger = logging.getLogger(__name__)


class BatchUpdateService:
    """Background service for processing batch updates"""
    
    def __init__(self, system_admin_id: int, check_interval: int = 300):
        self.system_admin_id = system_admin_id
        self.check_interval = check_interval  # 5 minutes default
        self._running = False
        self._lock = asyncio.Lock()
        
    def stop(self):
        """Signal the service to stop"""
        self._running = False
        
    async def run(self):
        """Main service loop"""
        self._running = True
        logger.info("Batch update service started")
        
        while self._running:
            try:
                # Use a separate session for each check cycle
                async with AsyncSessionLocal() as db:
                    await self._check_and_trigger_updates(db)
                    await db.commit()
            except Exception as e:
                logger.error(f"Error in batch update service loop: {e}", exc_info=True)
                # Continue running even if there's an error
                
            # Wait for next check interval
            try:
                await asyncio.sleep(self.check_interval)
            except asyncio.CancelledError:
                logger.info("Batch update service cancelled")
                break
                
        logger.info("Batch update service stopped")
    
    async def _check_and_trigger_updates(self, db: AsyncSession):
        """Check for batches that need updates and trigger them"""
        try:
            current_time = datetime.now(tz=timezone.utc)
            logger.debug(f"Checking for batch updates at {current_time}")
            
            # Get batches due for update
            batches = await batch_crud.get_batches_due_for_update(db, current_time)
            logger.info(f"Found {len(batches)} batches due for update")
            
            # Process each batch
            for batch in batches:
                # Use lock to prevent concurrent updates of the same batch
                async with self._lock:
                    try:
                        logger.info(f"Processing batch {batch.id}: {batch.name}")
                        await self._process_batch_update(db, batch.id)
                        
                        # Update last check time
                        batch.last_update_check = current_time
                        await db.flush()
                        
                    except Exception as e:
                        logger.error(f"Error processing batch {batch.id}: {e}")
                        # Continue with other batches
                        
        except Exception as e:
            logger.error(f"Error in check_and_trigger_updates: {e}")
            await self._log_error(
                db,
                "batch_update_service",
                "system",
                0,
                f"Batch update service error: {str(e)}"
            )
    
    async def _process_batch_update(self, db: AsyncSession, batch_id: int):
        """Process updates for a single batch"""
        try:
            # Get batch with fresh data
            batch = await batch_crud.get_batch(db, batch_id)
            if not batch:
                logger.error(f"Batch {batch_id} not found")
                return
                
            # Skip if batch is already updating
            if batch.update_state == BatchUpdateState.UPDATING:
                logger.info(f"Batch {batch_id} is already updating, skipping")
                return
            
            # Check if batch has active firmware assigned
            active_firmware = await batch_crud.get_active_firmware_for_batch(db, batch_id)
            if not active_firmware:
                logger.info(f"No active firmware for batch {batch_id}")
                return
            
            # Update batch state to UPDATING
            logger.info(f"Starting update for batch {batch_id}")
            batch = await batch_crud.update_batch_state(
                db,
                batch_id,
                BatchUpdateState.UPDATING,
                self.system_admin_id
            )
            
            # Get all devices in batch that need updates
            devices = await device_crud.get_devices_needing_update(
                db, 
                batch_id=batch_id,
                target_version=active_firmware.version
            )
            
            logger.info(f"Found {len(devices)} devices needing update in batch {batch_id}")
            
            # Track update results
            total_devices = len(devices)
            successful_updates = 0
            failed_updates = 0
            
            # Process each device asynchronously
            update_tasks = []
            for device in devices:
                task = self._update_device(db, device.id, active_firmware.id)
                update_tasks.append(task)
            
            # Wait for all updates to complete with timeout
            if update_tasks:
                results = await asyncio.gather(*update_tasks, return_exceptions=True)
                
                for result in results:
                    if isinstance(result, Exception):
                        failed_updates += 1
                        logger.error(f"Device update failed: {result}")
                    elif result:
                        successful_updates += 1
                    else:
                        failed_updates += 1
            
            # Update batch statistics
            batch.devices_total = total_devices
            batch.devices_updated = successful_updates
            batch.devices_failed = failed_updates
            
            # Determine final state
            if failed_updates == 0 and successful_updates > 0:
                final_state = BatchUpdateState.COMPLETED
            elif successful_updates == 0:
                final_state = BatchUpdateState.FAILED
            else:
                final_state = BatchUpdateState.COMPLETED  # Partial success
            
            # Update batch state
            await batch_crud.update_batch_state(
                db,
                batch_id,
                final_state,
                self.system_admin_id
            )
            
            logger.info(
                f"Batch {batch_id} update completed. "
                f"Success: {successful_updates}, Failed: {failed_updates}"
            )
            
        except Exception as e:
            logger.error(f"Error processing batch {batch_id}: {e}", exc_info=True)
            
            # Try to update batch state to FAILED
            try:
                await batch_crud.update_batch_state(
                    db,
                    batch_id,
                    BatchUpdateState.FAILED,
                    self.system_admin_id
                )
            except Exception as state_error:
                logger.error(f"Failed to update batch state: {state_error}")
            
            # Log the error
            await self._log_error(
                db,
                "batch_update",
                "batch",
                batch_id,
                f"Batch update failed: {str(e)}"
            )
    
    async def _update_device(self, db: AsyncSession, device_id: int, firmware_id: int) -> bool:
        """
        Simulate device update process
        In real implementation, this would notify the device
        Returns True if successful, False otherwise
        """
        try:
            # In a real system, this would:
            # 1. Send notification to device
            # 2. Wait for device to download and install
            # 3. Receive status update from device
            
            # For now, we'll simulate the update
            device = await device_crud.get_device(db, device_id)
            if not device:
                return False
            
            # Simulate update (in production, this would be triggered by device API calls)
            # For testing, you can manually call the OTA endpoints to simulate device behavior
            
            logger.info(f"Device {device_id} queued for update to firmware {firmware_id}")
            
            # Note: Actual update status will be reported by device via /api/ota/update-status
            return True
            
        except Exception as e:
            logger.error(f"Error updating device {device_id}: {e}")
            return False
    
    async def _log_error(
        self, 
        db: AsyncSession,
        action_type: str,
        entity_type: str,
        entity_id: int,
        details: str
    ):
        """Log an error to the admin logs"""
        try:
            await logs_crud.create_admin_log(
                db,
                AdminLogCreate(
                    admin_id=self.system_admin_id,
                    action_type=action_type,
                    entity_type=entity_type,
                    entity_id=entity_id,
                    details=details,
                    level=LogLevel.ERROR
                )
            )
            await db.flush()
        except Exception as e:
            logger.error(f"Failed to create error log: {e}")


# Legacy function for backward compatibility
async def run_batch_update_service(db: AsyncSession, system_admin_id: int):
    """Legacy function - use BatchUpdateService class instead"""
    service = BatchUpdateService(system_admin_id)
    await service.run()