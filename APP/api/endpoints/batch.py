from fastapi import APIRouter, Depends, HTTPException, status
from sqlalchemy.ext.asyncio import AsyncSession
from typing import List
from ...db.sessions import get_session
from ...crud import batch as batch_crud
from ...crud import device as device_crud
from ...schemas.batch import (
    BatchCreate, BatchUpdate, BatchResponse, 
    BatchFirmwareAssign, BatchUpdateTrigger
)
from ...schemas.device import DeviceResponse
from ...models.batch import BatchUpdateState
from ...api.endpoints.auth import check_admin_role, get_current_admin

router = APIRouter()

@router.post("/", response_model=BatchResponse, status_code=status.HTTP_201_CREATED)
async def create_batch(
    batch: BatchCreate,
    db: AsyncSession = Depends(get_session)
):
    return await batch_crud.create_batch(db, batch)

@router.get("/", response_model=List[BatchResponse])
async def list_batches(
    skip: int = 0,
    limit: int = 100,
    db: AsyncSession = Depends(get_session),
    _: str = Depends(check_admin_role(["admin", "manager", "viewer"]))
):
    return await batch_crud.get_batches(db, skip=skip, limit=limit)

@router.get("/{batch_id}", response_model=BatchResponse)
async def get_batch(
    batch_id: int,
    db: AsyncSession = Depends(get_session),
    _: str = Depends(check_admin_role(["admin", "manager", "viewer"]))
):
    batch = await batch_crud.get_batch(db, batch_id)
    if not batch:
        raise HTTPException(status_code=404, detail="Batch not found")
    return batch

@router.put("/{batch_id}", response_model=BatchResponse)
async def update_batch(
    batch_id: int,
    batch_update: BatchUpdate,
    db: AsyncSession = Depends(get_session),
    current_admin = Depends(check_admin_role(["admin", "manager"]))
):
    batch = await batch_crud.get_batch(db, batch_id)
    if not batch:
        raise HTTPException(status_code=404, detail="Batch not found")
    return await batch_crud.update_batch(db, batch, batch_update)

@router.delete("/{batch_id}", status_code=status.HTTP_204_NO_CONTENT)
async def delete_batch(
    batch_id: int,
    db: AsyncSession = Depends(get_session),
    current_admin = Depends(check_admin_role(["admin"]))
):
    batch = await batch_crud.get_batch(db, batch_id)
    if not batch:
        raise HTTPException(status_code=404, detail="Batch not found")
    await batch_crud.delete_batch(db, batch)

@router.post("/{batch_id}/assign-firmware", status_code=status.HTTP_200_OK)
async def assign_firmware_to_batch(
    batch_id: int,
    firmware_assign: BatchFirmwareAssign,
    db: AsyncSession = Depends(get_session),
    current_admin = Depends(check_admin_role(["admin", "manager"]))
):
    batch = await batch_crud.get_batch(db, batch_id)
    if not batch:
        raise HTTPException(status_code=404, detail="Batch not found")
    
    try:
        await batch_crud.assign_firmware(
            db, 
            batch_id, 
            firmware_assign,
            current_admin.id
        )
    except Exception as e:
        raise HTTPException(
            status_code=400,
            detail=f"Could not assign firmware: {str(e)}"
        )
    return {"status": "success"}

@router.get("/{batch_id}/devices", response_model=List[DeviceResponse])
async def get_batch_devices(
    batch_id: int,
    skip: int = 0,
    limit: int = 100,
    db: AsyncSession = Depends(get_session),
    _: str = Depends(check_admin_role(["admin", "manager", "viewer"]))
):
    batch = await batch_crud.get_batch(db, batch_id)
    if not batch:
        raise HTTPException(status_code=404, detail="Batch not found")
    
    return await device_crud.get_devices(
        db, 
        skip=skip,
        limit=limit,
        batch_id=batch_id
    )

@router.post("/{batch_id}/trigger-update")
async def trigger_batch_update(
    batch_id: int,
    trigger: BatchUpdateTrigger,
    db: AsyncSession = Depends(get_session),
    current_admin = Depends(check_admin_role(["admin", "manager"]))
):
    batch = await batch_crud.get_batch(db, batch_id)
    if not batch:
        raise HTTPException(status_code=404, detail="Batch not found")
    
    if batch.update_state == BatchUpdateState.UPDATING:
        raise HTTPException(
            status_code=400,
            detail="Batch is already updating"
        )
    
    updated_batch = await batch_crud.update_batch_state(
        db,
        batch_id,
        BatchUpdateState.UPDATING,
        current_admin.id,
        force=trigger.force
    )
    
    return {
        "status": "update_triggered",
        "batch_state": updated_batch.update_state,
        "devices_total": updated_batch.devices_total
    }

@router.get("/{batch_id}/status")
async def get_batch_update_status(
    batch_id: int,
    db: AsyncSession = Depends(get_session),
    _: str = Depends(check_admin_role(["admin", "manager", "viewer"]))
):
    batch = await batch_crud.get_batch(db, batch_id)
    if not batch:
        raise HTTPException(status_code=404, detail="Batch not found")
    
    total, updated, failed = await batch_crud.get_batch_statistics(db, batch_id)
    
    return {
        "state": batch.update_state,
        "last_update_check": batch.last_update_check,
        "last_update_trigger": batch.last_update_trigger,
        "devices_total": total,
        "devices_updated": updated,
        "devices_failed": failed,
        "progress": round((updated + failed) * 100 / total) if total > 0 else 0
    }