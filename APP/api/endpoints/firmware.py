from fastapi import APIRouter, Depends, Form, HTTPException, status, UploadFile, File
from fastapi.responses import FileResponse
from sqlalchemy.ext.asyncio import AsyncSession
from typing import List, Optional
import os

from ...db.sessions import get_session
from ...crud import firmware as firmware_crud
from ...schemas.firmware import FirmwareCreate, FirmwareUpdate, FirmwareResponse
from ...core.config import settings

router = APIRouter()

@router.post("/", response_model=FirmwareResponse, status_code=status.HTTP_201_CREATED)
async def upload_firmware(
    version: str = Form(...),
    update_name: str = Form(...),
    release_notes: Optional[str] = Form(None),
    is_beta: bool = Form(False),
    file: UploadFile = File(...),
    db: AsyncSession = Depends(get_session)
):
    # Check if version already exists
    existing_firmware = await firmware_crud.get_firmware_by_version(db, version)
    if existing_firmware:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail="Firmware version already exists"
        )

    # Ensure storage directory exists
    os.makedirs(settings.FIRMWARE_STORAGE_PATH, exist_ok=True)
    file_path = os.path.join(settings.FIRMWARE_STORAGE_PATH, f"{version}.bin")

    # Save file
    with open(file_path, "wb") as f:
        content = await file.read()
        f.write(content)
        file_size = len(content)

    firmware_data = FirmwareCreate(
    version=version,
    update_name=update_name,
    release_notes=release_notes,
    is_beta=is_beta
)
    return await firmware_crud.create_firmware(db, firmware_data, file_path, file_size)

@router.get("/", response_model=List[FirmwareResponse])
async def list_firmware(
    skip: int = 0,
    limit: int = 100,
    include_beta: bool = False,
    db: AsyncSession = Depends(get_session)
):
    return await firmware_crud.get_firmwares(db, skip=skip, limit=limit, include_beta=include_beta)

@router.get("/{firmware_id}", response_model=FirmwareResponse)
async def get_firmware(firmware_id: int, db: AsyncSession = Depends(get_session)):
    firmware = await firmware_crud.get_firmware(db, firmware_id)
    if not firmware:
        raise HTTPException(status_code=404, detail="Firmware not found")
    return firmware

@router.get("/{firmware_id}/download")
async def download_firmware(firmware_id: int, db: AsyncSession = Depends(get_session)):
    firmware = await firmware_crud.get_firmware(db, firmware_id)
    if not firmware:
        raise HTTPException(status_code=404, detail="Firmware not found")
    
    if not os.path.exists(firmware.file_path):
        raise HTTPException(status_code=404, detail="Firmware file not found")
    
    return FileResponse(
        firmware.file_path,
        media_type='application/octet-stream',
        filename=f"{firmware.version}.bin"
    )

@router.put("/{firmware_id}", response_model=FirmwareResponse)
async def update_firmware(
    firmware_id: int,
    firmware_update: FirmwareUpdate,
    db: AsyncSession = Depends(get_session)
):
    firmware = await firmware_crud.get_firmware(db, firmware_id)
    if not firmware:
        raise HTTPException(status_code=404, detail="Firmware not found")
    return await firmware_crud.update_firmware(db, firmware, firmware_update)

@router.delete("/{firmware_id}", status_code=status.HTTP_204_NO_CONTENT)
async def delete_firmware(firmware_id: int, db: AsyncSession = Depends(get_session)):
    firmware = await firmware_crud.get_firmware(db, firmware_id)
    if not firmware:
        raise HTTPException(status_code=404, detail="Firmware not found")
    
    # Delete file if it exists
    if os.path.exists(firmware.file_path):
        os.remove(firmware.file_path)
        
    await firmware_crud.delete_firmware(db, firmware)