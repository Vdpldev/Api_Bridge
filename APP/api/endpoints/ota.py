from threading import stack_size
from fastapi import APIRouter, Depends, HTTPException, status, Query
from sqlalchemy.ext.asyncio import AsyncSession
from sqlalchemy import select
from datetime import datetime, timezone
from typing import Optional
import logging
from starlette import status as http_status 
from APP.db.sessions import get_session
from APP.crud import device as device_crud
from APP.crud import firmware as firmware_crud
from APP.crud import logs as logs_crud
from APP.models.logs import DeviceUpdateLog
from APP.schemas.logs import UpdateLogCreate, LogLevel
from APP.models.firmware import Firmware, BatchFirmware
from packaging.version import parse as parse_version 
from APP.schemas.ota import UpdateStatusRequest

router = APIRouter()
logger = logging.getLogger(__name__)


@router.get("/checkupdate")
async def check_update(
    mac_address: str = Query(..., description="Device MAC address"),
    current_version: str = Query(..., description="Current firmware version"),
    db: AsyncSession = Depends(get_session)
):
    """
    Check if a firmware update is available for the device.
    This endpoint is called periodically by devices.
    """
    try:
        # Get device by MAC address
        device = await device_crud.get_device_by_mac(db, mac_address)
        if not device:
            logger.warning(f"Unknown device checking for update: {mac_address}")
            raise HTTPException(
                status_code=status.HTTP_404_NOT_FOUND,
                detail="Device not found. Please register the device first."
            )

        # Update last check time
        device.last_check_time = datetime.now(tz=timezone.utc)
        await db.flush()

        # Check if device is assigned to a batch
        if not device.batch_id:
            logger.info(f"Device {mac_address} has no batch assigned")
            return {
                "update_available": False,
                "reason": "Device not assigned to any batch"
            }

        # --- ORM QUERY REPLACEMENT ---
        # Get the active firmware for the device's batch using the ORM
        stmt = (
            select(Firmware)
            .join(BatchFirmware, Firmware.id == BatchFirmware.firmware_id)
            .where(
                BatchFirmware.batch_id == device.batch_id,
                BatchFirmware.is_active == True
            )
            .order_by(Firmware.created_at.desc())
            .limit(1)
        )
        
        result = await db.execute(stmt)
        latest_firmware = result.scalar_one_or_none()
        # --- END REPLACEMENT ---

        if not latest_firmware:
            logger.info(f"No active firmware for batch {device.batch_id}")
            return {
                "update_available": False,
                "reason": "No firmware assigned to device's batch"
            }

        # --- SEMANTIC VERSION COMPARISON ---
        # Compare versions using the packaging library for correctness
        if parse_version(latest_firmware.version) <= parse_version(current_version):
            logger.info(
                f"Device {mac_address} already on latest version "
                f"({current_version} >= {latest_firmware.version})"
            )
            return {
                "update_available": False,
                "reason": "Device already on latest or newer version",
                "current_version": current_version,
                "latest_version": latest_firmware.version
            }

        # Update available
        logger.info(
            f"Update available for device {mac_address}: "
            f"{current_version} -> {latest_firmware.version}"
        )
        
        # Log the update check
        log = UpdateLogCreate(
            device_id=device.id,
            firmware_id=latest_firmware.id,
            status="update_available",
            level=LogLevel.INFO
        )
        await logs_crud.create_update_log(db, log)

        return {
            "update_available": True,
            "firmware_id": latest_firmware.id,
            "version": latest_firmware.version,
            "size": latest_firmware.file_size,
            "checksum": latest_firmware.checksum,
            "update_name": latest_firmware.update_name,
            "release_notes": latest_firmware.release_notes,
            "is_beta": latest_firmware.is_beta
        }

    except HTTPException:
        raise
    except Exception as e:
        logger.error(f"Error checking update for device {mac_address}: {e}", exc_info=True)
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail="Internal server error while checking for updates"
        )



@router.post("/update-status")
async def update_status(
    status_update: UpdateStatusRequest,
    db: AsyncSession = Depends(get_session)
):
    """
    Report firmware update status from device.
    Status can be: downloading, installing, success, failed
    """
    try:
        mac_address = status_update.mac_address
        firmware_id = status_update.firmware_id
        status = status_update.status # This 'status' is the string for firmware update status
        error_message = status_update.error_message

        logger.info(
            f"Received update status from device {mac_address}: "
            f"firmware_id={firmware_id}, status={status}, error_message={error_message}"
        )

        # Validate status
        valid_statuses = ["downloading", "installing", "success", "failed"]
        if status not in valid_statuses:
            logger.error(f"Invalid status '{status}' for device {mac_address}")
            raise HTTPException(
                status_code=http_status.HTTP_400_BAD_REQUEST, # Use the aliased http_status
                detail=f"Invalid status. Must be one of: {valid_statuses}"
            )

        # Get device
        device = await device_crud.get_device_by_mac(db, mac_address)
        if not device:
            logger.warning(f"Unknown device reporting status: {mac_address}")
            raise HTTPException(
                status_code=http_status.HTTP_404_NOT_FOUND, # Use the aliased http_status
                detail="Device not found"
            )

        # Get firmware
        firmware = await firmware_crud.get_firmware(db, firmware_id)
        if not firmware:
            logger.error(f"Firmware {firmware_id} not found for device {mac_address}")
            raise HTTPException(
                status_code=http_status.HTTP_404_NOT_FOUND, # Use the aliased http_status
                detail="Firmware not found"
            )

        # Determine log level based on status
        if status == "failed":
            log_level = LogLevel.ERROR
        elif status in ["downloading", "installing"]:
            log_level = LogLevel.INFO
        else:  # success
            log_level = LogLevel.INFO

        # Create update log
        log = DeviceUpdateLog(
            device_id=device.id,
            firmware_id=firmware_id,
            status=status,
            error_message=error_message,
            level=log_level
        )
        db.add(log)

        # Update device status based on update result
        if status == "success":
            device.current_firmware_version = firmware.version
            device.last_update_time = datetime.now(tz=timezone.utc)
            device.status = "updated"
            logger.info(
                f"Device {mac_address} successfully updated to version {firmware.version}"
            )
        elif status == "failed":
            device.status = "failed"
            logger.error(
                f"Device {mac_address} failed to update: {error_message}"
            )
        # For downloading/installing, keep current status

        await db.flush()

        return {
            "status": "logged",
            "device_status": device.status,
            "message": f"Update status '{status}' recorded for device {mac_address}"
        }

    except HTTPException:
        raise
    except Exception as e:
        logger.error(f"Error updating status for device {mac_address}: {e}", exc_info=True)
        raise HTTPException(
            status_code=http_status.HTTP_500_INTERNAL_SERVER_ERROR, # Use the aliased http_status
            detail="Internal server error while recording update status"
        )

@router.get("/device-info/{mac_address}")
async def get_device_info(
    mac_address: str,
    db: AsyncSession = Depends(get_session)
):
    """Get device information and update history"""
    try:
        # Get device
        device = await device_crud.get_device_by_mac(db, mac_address)
        if not device:
            raise HTTPException(
                status_code=status.HTTP_404_NOT_FOUND,
                detail="Device not found"
            )

        # Get recent update logs
        logs = await logs_crud.get_device_logs(
            db,
            device_id=device.id,
            limit=10
        )

        return {
            "device": {
                "id": device.id,
                "mac_address": device.mac_address,
                "current_firmware_version": device.current_firmware_version,
                "batch_id": device.batch_id,
                "status": device.status,
                "last_check_time": device.last_check_time,
                "last_update_time": device.last_update_time,
                "created_at": device.created_at
            },
            "recent_logs": [
                {
                    "id": log.id,
                    "firmware_id": log.firmware_id,
                    "status": log.status,
                    "error_message": log.error_message,
                    "created_at": log.created_at
                }
                for log in logs
            ]
        }

    except HTTPException:
        raise
    except Exception as e:
        logger.error(f"Error getting device info for {mac_address}: {e}", exc_info=True)
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail="Internal server error while retrieving device information"
        )

# TODO: For production, consider adding:
# 1. Device authentication (API key, certificates)
# 2. Rate limiting per device
# 3. Bandwidth throttling for firmware downloads
# 4. Update scheduling to avoid overwhelming the server
# 5. Delta updates for bandwidth efficiency
# 6. Rollback capability for failed updates