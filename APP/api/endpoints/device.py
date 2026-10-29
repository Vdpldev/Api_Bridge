from fastapi import APIRouter, Depends, HTTPException, status
from sqlalchemy.ext.asyncio import AsyncSession
from typing import List
from ...db.sessions import get_session
from ...crud import device as device_crud
from ...schemas.device import DeviceCreate, DeviceUpdate, DeviceResponse

router = APIRouter()

@router.post("/register", response_model=DeviceResponse, status_code=status.HTTP_201_CREATED)
async def register_device(
    device: DeviceCreate,
    db: AsyncSession = Depends(get_session)
):
    existing_device = await device_crud.get_device_by_mac(db, device.mac_address)
    if existing_device:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail="Device with this MAC address already exists"
        )
    return await device_crud.create_device(db, device)

@router.get("/", response_model=List[DeviceResponse])
async def list_devices(
    skip: int = 0,
    limit: int = 100,
    batch_id: int = None,
    db: AsyncSession = Depends(get_session)
):
    devices = await device_crud.get_devices(db, skip=skip, limit=limit, batch_id=batch_id)
    return devices

@router.get("/{device_id}", response_model=DeviceResponse)
async def get_device(device_id: int, db: AsyncSession = Depends(get_session)):
    device = await device_crud.get_device(db, device_id)
    if not device:
        raise HTTPException(status_code=404, detail="Device not found")
    return device

@router.put("/{device_id}", response_model=DeviceResponse)
async def update_device(
    device_id: int,
    device_update: DeviceUpdate,
    db: AsyncSession = Depends(get_session)
):
    device = await device_crud.get_device(db, device_id)
    if not device:
        raise HTTPException(status_code=404, detail="Device not found")
    return await device_crud.update_device(db, device, device_update)

@router.delete("/{device_id}", status_code=status.HTTP_204_NO_CONTENT)
async def delete_device(device_id: int, db: AsyncSession = Depends(get_session)):
    device = await device_crud.get_device(db, device_id)
    if not device:
        raise HTTPException(status_code=404, detail="Device not found")
    await device_crud.delete_device(db, device)