from sqlalchemy.ext.asyncio import AsyncSession
from sqlalchemy import select, update, and_, or_
from typing import Optional, List
from APP.models.devices import Device
from APP.schemas.device import DeviceCreate, DeviceUpdate
import logging

logger = logging.getLogger(__name__)

async def create_device(
    db: AsyncSession,
    device: DeviceCreate
) -> Device:
    """Create a new device"""
    db_device = Device(
        mac_address=device.mac_address,
        current_firmware_version=device.current_firmware_version,
        batch_id=device.batch_id,
        status=device.status
    )
    db.add(db_device)
    await db.flush()
    await db.refresh(db_device)
    logger.info(f"Created device {db_device.id} with MAC {device.mac_address}")
    return db_device

async def get_device(db: AsyncSession, device_id: int) -> Optional[Device]:
    """Get device by ID"""
    result = await db.execute(select(Device).filter(Device.id == device_id))
    return result.scalar_one_or_none()

async def get_device_by_mac(db: AsyncSession, mac_address: str) -> Optional[Device]:
    """Get device by MAC address"""
    result = await db.execute(
        select(Device).filter(Device.mac_address == mac_address)
    )
    return result.scalar_one_or_none()

async def get_devices(
    db: AsyncSession, 
    skip: int = 0, 
    limit: int = 100,
    batch_id: Optional[int] = None,
    status: Optional[str] = None
) -> List[Device]:
    """Get list of devices with optional filtering"""
    query = select(Device)
    
    # Apply filters
    if batch_id is not None:
        query = query.filter(Device.batch_id == batch_id)
    if status is not None:
        query = query.filter(Device.status == status)
    
    # Apply pagination
    query = query.offset(skip).limit(limit)
    
    result = await db.execute(query)
    return result.scalars().all()

async def update_device(
    db: AsyncSession,
    device: Device,
    device_update: DeviceUpdate
) -> Device:
    """Update device information"""
    update_data = device_update.dict(exclude_unset=True)
    for field, value in update_data.items():
        setattr(device, field, value)
    
    await db.flush()
    await db.refresh(device)
    logger.info(f"Updated device {device.id}")
    return device

async def delete_device(db: AsyncSession, device: Device) -> None:
    """Delete a device"""
    device_id = device.id
    await db.delete(device)
    await db.flush()
    logger.info(f"Deleted device {device_id}")

async def get_devices_needing_update(
    db: AsyncSession,
    batch_id: int,
    target_version: str,
    limit: Optional[int] = None
) -> List[Device]:
    """Get devices in a batch that need updating to target version"""
    query = select(Device).filter(
        and_(
            Device.batch_id == batch_id,
            Device.current_firmware_version != target_version,
            Device.status.in_(['active', 'failed'])  # Include failed devices for retry
        )
    )
    
    if limit:
        query = query.limit(limit)
    
    result = await db.execute(query)
    return result.scalars().all()

async def update_device_status(
    db: AsyncSession,
    device_id: int,
    status: str,
    firmware_version: Optional[str] = None
) -> Optional[Device]:
    """Update device status and optionally firmware version"""
    device = await get_device(db, device_id)
    if not device:
        return None
    
    device.status = status
    if firmware_version:
        device.current_firmware_version = firmware_version
    
    await db.flush()
    await db.refresh(device)
    logger.info(f"Updated device {device_id} status to {status}")
    return device

async def get_device_statistics(db: AsyncSession) -> dict:
    """Get overall device statistics"""
    # Total devices
    total_result = await db.execute(
        select(func.count(Device.id))
    )
    total = total_result.scalar() or 0
    
    # Devices by status
    status_result = await db.execute(
        select(Device.status, func.count(Device.id))
        .group_by(Device.status)
    )
    status_counts = dict(status_result.all())
    
    # Devices by batch
    batch_result = await db.execute(
        select(Device.batch_id, func.count(Device.id))
        .group_by(Device.batch_id)
    )
    batch_counts = dict(batch_result.all())
    
    return {
        "total": total,
        "by_status": status_counts,
        "by_batch": batch_counts,
        "active": status_counts.get('active', 0),
        "updated": status_counts.get('updated', 0),
        "failed": status_counts.get('failed', 0)
    }

async def bulk_update_device_batch(
    db: AsyncSession,
    device_ids: List[int],
    new_batch_id: int
) -> int:
    """Move multiple devices to a new batch"""
    result = await db.execute(
        update(Device)
        .where(Device.id.in_(device_ids))
        .values(batch_id=new_batch_id)
    )
    await db.flush()
    
    affected = result.rowcount
    logger.info(f"Moved {affected} devices to batch {new_batch_id}")
    return affected

async def get_devices_by_firmware_version(
    db: AsyncSession,
    firmware_version: str,
    skip: int = 0,
    limit: int = 100
) -> List[Device]:
    """Get all devices running a specific firmware version"""
    query = (
        select(Device)
        .filter(Device.current_firmware_version == firmware_version)
        .offset(skip)
        .limit(limit)
    )
    result = await db.execute(query)
    return result.scalars().all()

async def get_outdated_devices(
    db: AsyncSession,
    skip: int = 0,
    limit: int = 100
) -> List[Device]:
    """Get devices that are not on their batch's active firmware"""
    from APP.models.firmware import Firmware, BatchFirmware
    
    # Complex query to find devices not on their batch's active firmware
    query = (
        select(Device)
        .join(BatchFirmware, Device.batch_id == BatchFirmware.batch_id)
        .join(Firmware, BatchFirmware.firmware_id == Firmware.id)
        .filter(
            and_(
                BatchFirmware.is_active == True,
                Device.current_firmware_version != Firmware.version
            )
        )
        .offset(skip)
        .limit(limit)
    )
    
    result = await db.execute(query)
    return result.scalars().all()

# Add missing import at the top
from sqlalchemy import func