from sqlalchemy.ext.asyncio import AsyncSession
from sqlalchemy import select, and_, func, text, or_
from sqlalchemy.orm import selectinload
from typing import List, Optional, Tuple
from datetime import datetime, timedelta, timezone
from APP.models.batch import Batch, BatchUpdateState
from APP.models.devices import Device
from APP.models.firmware import BatchFirmware, Firmware
from APP.schemas.batch import BatchCreate, BatchUpdate, BatchFirmwareAssign
from APP.crud import logs as logs_crud
from APP.schemas.logs import AdminLogCreate, LogLevel
import logging

logger = logging.getLogger(__name__)

async def create_batch(db: AsyncSession, batch: BatchCreate) -> Batch:
    """Create a new batch"""
    db_batch = Batch(**batch.dict())
    db.add(db_batch)
    await db.flush()
    await db.refresh(db_batch)
    return db_batch

async def get_batch(db: AsyncSession, batch_id: int) -> Optional[Batch]:
    """Get batch by ID"""
    result = await db.execute(select(Batch).filter(Batch.id == batch_id))
    return result.scalar_one_or_none()

async def get_batches(
    db: AsyncSession,
    skip: int = 0,
    limit: int = 100
) -> List[Batch]:
    """Get list of batches with pagination"""
    query = select(Batch).offset(skip).limit(limit)
    result = await db.execute(query)
    return result.scalars().all()

async def update_batch(
    db: AsyncSession,
    batch: Batch,
    batch_update: BatchUpdate
) -> Batch:
    """Update batch details"""
    update_data = batch_update.dict(exclude_unset=True)
    for field, value in update_data.items():
        setattr(batch, field, value)
    await db.flush()
    await db.refresh(batch)
    return batch

async def delete_batch(db: AsyncSession, batch: Batch) -> None:
    """Delete a batch"""
    await db.delete(batch)
    await db.flush()

async def assign_firmware(
    db: AsyncSession,
    batch_id: int,
    firmware_assign: BatchFirmwareAssign,
    admin_id: int
) -> BatchFirmware:
    """Assign firmware to a batch"""
    # Deactivate any currently active firmware for this batch
    if firmware_assign.is_active:
        await db.execute(
            text("""UPDATE batch_firmware
            SET is_active = false
            WHERE batch_id = :batch_id AND is_active = true"""),
            {"batch_id": batch_id}
        )

    # Check if this firmware is already assigned to this batch
    existing = await db.execute(
        select(BatchFirmware).filter(
            and_(
                BatchFirmware.batch_id == batch_id,
                BatchFirmware.firmware_id == firmware_assign.firmware_id
            )
        )
    )
    db_batch_firmware = existing.scalar_one_or_none()
    
    if db_batch_firmware:
        # Update existing assignment
        db_batch_firmware.is_active = firmware_assign.is_active
    else:
        # Create new batch-firmware association
        db_batch_firmware = BatchFirmware(
            batch_id=batch_id,
            firmware_id=firmware_assign.firmware_id,
            is_active=firmware_assign.is_active
        )
        db.add(db_batch_firmware)

    # Log the firmware assignment
    log = AdminLogCreate(
        admin_id=admin_id,
        action_type="firmware_assign",
        entity_type="batch",
        entity_id=batch_id,
        details=f"Assigned firmware {firmware_assign.firmware_id} to batch {batch_id}",
        level=LogLevel.INFO
    )
    await logs_crud.create_admin_log(db, log)

    await db.flush()
    await db.refresh(db_batch_firmware)
    return db_batch_firmware

async def get_batch_statistics(
    db: AsyncSession,
    batch_id: int
) -> Tuple[int, int, int]:
    """Get the count of total, updated, and failed devices in a batch"""
    # Get total count
    total_result = await db.execute(
        select(func.count(Device.id)).filter(Device.batch_id == batch_id)
    )
    total = total_result.scalar() or 0

    # Get updated count
    updated_result = await db.execute(
        select(func.count(Device.id)).filter(
            and_(
                Device.batch_id == batch_id,
                Device.status == 'updated'
            )
        )
    )
    updated = updated_result.scalar() or 0

    # Get failed count
    failed_result = await db.execute(
        select(func.count(Device.id)).filter(
            and_(
                Device.batch_id == batch_id,
                Device.status == 'failed'
            )
        )
    )
    failed = failed_result.scalar() or 0

    return total, updated, failed

async def update_batch_state(
    db: AsyncSession,
    batch_id: int,
    new_state: BatchUpdateState,
    admin_id: int,
    force: bool = False
) -> Batch:
    """Update batch state and related timestamps"""
    batch = await get_batch(db, batch_id)
    if not batch:
        return None

    old_state = batch.update_state

    if new_state == BatchUpdateState.UPDATING:
        batch.last_update_trigger = datetime.now(tz=timezone.utc)
        # Update statistics
        total, updated, failed = await get_batch_statistics(db, batch_id)
        batch.devices_total = total
        batch.devices_updated = updated
        batch.devices_failed = failed

    batch.update_state = new_state

    # Log the state change
    log = AdminLogCreate(
        admin_id=admin_id,
        action_type="batch_state_change",
        entity_type="batch",
        entity_id=batch_id,
        details=f"Batch state changed from {old_state} to {new_state}. Force update: {force}",
        level=LogLevel.INFO
    )
    await logs_crud.create_admin_log(db, log)

    await db.flush()
    await db.refresh(batch)
    return batch

async def get_batches_due_for_update(
    db: AsyncSession,
    current_time: datetime
) -> List[Batch]:
    """Get all batches that are due for an update check"""
    # Calculate the time window for checking
    # A batch is due if:
    # 1. auto_update is enabled
    # 2. It's in IDLE state
    # 3. check_update_time is in the past
    # 4. Either never checked or last check was more than 24 hours ago
    
    twenty_four_hours_ago = current_time - timedelta(hours=24)
    
    query = select(Batch).filter(
        and_(
            Batch.auto_update == True,
            Batch.update_state == BatchUpdateState.IDLE,
            Batch.check_update_time <= current_time,
            or_(
                Batch.last_update_check == None,
                Batch.last_update_check <= twenty_four_hours_ago
            )
        )
    )
    result = await db.execute(query)
    return result.scalars().all()

async def get_active_firmware_for_batch(
    db: AsyncSession,
    batch_id: int
) -> Optional[Firmware]:
    """Get the active firmware assigned to a batch"""
    query = (
        select(Firmware)
        .join(BatchFirmware)
        .filter(
            and_(
                BatchFirmware.batch_id == batch_id,
                BatchFirmware.is_active == True
            )
        )
    )
    result = await db.execute(query)
    return result.scalar_one_or_none()

async def get_batch_firmware_history(
    db: AsyncSession,
    batch_id: int
) -> List[Tuple[Firmware, BatchFirmware]]:
    """Get all firmware assignments for a batch"""
    query = (
        select(Firmware, BatchFirmware)
        .join(BatchFirmware)
        .filter(BatchFirmware.batch_id == batch_id)
        .order_by(BatchFirmware.created_at.desc())
    )
    result = await db.execute(query)
    return result.all()

async def get_batches_by_firmware(
    db: AsyncSession,
    firmware_id: int,
    only_active: bool = True
) -> List[Batch]:
    """Get all batches that have a specific firmware assigned"""
    query = (
        select(Batch)
        .join(BatchFirmware)
        .filter(BatchFirmware.firmware_id == firmware_id)
    )
    
    if only_active:
        query = query.filter(BatchFirmware.is_active == True)
    
    result = await db.execute(query)
    return result.scalars().all()

async def get_batch_update_progress(
    db: AsyncSession,
    batch_id: int
) -> dict:
    """Get detailed update progress for a batch"""
    batch = await get_batch(db, batch_id)
    if not batch:
        return None
    
    # Get device statistics
    total, updated, failed = await get_batch_statistics(db, batch_id)
    
    # Get active firmware
    active_firmware = await get_active_firmware_for_batch(db, batch_id)
    
    # Calculate progress percentage
    progress = 0
    if total > 0:
        progress = round((updated + failed) * 100 / total)
    
    return {
        "batch_id": batch_id,
        "batch_name": batch.name,
        "state": batch.update_state,
        "last_update_check": batch.last_update_check,
        "last_update_trigger": batch.last_update_trigger,
        "devices": {
            "total": total,
            "updated": updated,
            "failed": failed,
            "pending": total - updated - failed
        },
        "progress_percentage": progress,
        "active_firmware": {
            "id": active_firmware.id,
            "version": active_firmware.version,
            "name": active_firmware.update_name
        } if active_firmware else None
    }