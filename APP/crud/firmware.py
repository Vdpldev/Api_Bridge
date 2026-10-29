from sqlalchemy.ext.asyncio import AsyncSession
from sqlalchemy import select
from typing import Optional, List
from ..models.firmware import Firmware
from ..schemas.firmware import FirmwareCreate, FirmwareUpdate
import hashlib

async def create_firmware(
    db: AsyncSession,
    firmware: FirmwareCreate,
    file_path: str,
    file_size: int
) -> Firmware:
    # Calculate MD5 checksum for firmware integrity
    with open(file_path, "rb") as f:
        file_hash = hashlib.md5()
        while chunk := f.read(8192):
            file_hash.update(chunk)
        checksum = file_hash.hexdigest()

    db_firmware = Firmware(
        version=firmware.version,
        file_path=file_path,
        update_name=firmware.update_name,
        release_notes=firmware.release_notes,
        is_beta=firmware.is_beta,
        file_size=file_size,
        checksum=checksum
    )
    db.add(db_firmware)
    await db.flush()
    await db.refresh(db_firmware)
    return db_firmware

async def get_firmware(db: AsyncSession, firmware_id: int) -> Optional[Firmware]:
    result = await db.execute(select(Firmware).filter(Firmware.id == firmware_id))
    return result.scalar_one_or_none()

async def get_firmware_by_version(db: AsyncSession, version: str) -> Optional[Firmware]:
    result = await db.execute(select(Firmware).filter(Firmware.version == version))
    return result.scalar_one_or_none()

async def get_firmwares(
    db: AsyncSession,
    skip: int = 0,
    limit: int = 100,
    include_beta: bool = False
) -> List[Firmware]:
    query = select(Firmware)
    if not include_beta:
        query = query.filter(Firmware.is_beta == False)
    query = query.offset(skip).limit(limit)
    result = await db.execute(query)
    return result.scalars().all()

async def update_firmware(
    db: AsyncSession,
    firmware: Firmware,
    firmware_update: FirmwareUpdate
) -> Firmware:
    update_data = firmware_update.dict(exclude_unset=True)
    for field, value in update_data.items():
        setattr(firmware, field, value)
    await db.flush()
    await db.refresh(firmware)
    return firmware

async def delete_firmware(db: AsyncSession, firmware: Firmware) -> None:
    await db.delete(firmware)
    await db.flush()