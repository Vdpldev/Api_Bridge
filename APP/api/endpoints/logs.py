from fastapi import APIRouter, Depends, HTTPException, status, Query
from sqlalchemy.ext.asyncio import AsyncSession
from typing import List, Optional
from datetime import datetime
from APP.db.sessions import get_session
from APP.crud import logs as logs_crud
from APP.schemas.logs import UpdateLogInDB, AdminLogInDB, LogFilter
from APP.api.endpoints.auth import check_admin_role

router = APIRouter()

@router.get("/device-logs", response_model=List[UpdateLogInDB])
async def get_device_logs(
    device_id: Optional[int] = None,
    start_date: Optional[datetime] = None,
    end_date: Optional[datetime] = None,
    level: Optional[str] = None,
    skip: int = Query(0, ge=0),
    limit: int = Query(100, ge=1, le=1000),
    db: AsyncSession = Depends(get_session),
    _: str = Depends(check_admin_role(["admin", "manager", "viewer"]))
):
    log_filter = LogFilter(
        start_date=start_date,
        end_date=end_date,
        level=level
    )
    return await logs_crud.get_device_logs(
        db, 
        device_id=device_id,
        log_filter=log_filter,
        skip=skip,
        limit=limit
    )

@router.get("/admin-logs", response_model=List[AdminLogInDB])
async def get_admin_logs(
    start_date: Optional[datetime] = None,
    end_date: Optional[datetime] = None,
    level: Optional[str] = None,
    entity_type: Optional[str] = None,
    entity_id: Optional[int] = None,
    action_type: Optional[str] = None,
    skip: int = Query(0, ge=0),
    limit: int = Query(100, ge=1, le=1000),
    db: AsyncSession = Depends(get_session),
    _: str = Depends(check_admin_role(["admin"]))
):
    log_filter = LogFilter(
        start_date=start_date,
        end_date=end_date,
        level=level,
        entity_type=entity_type,
        entity_id=entity_id,
        action_type=action_type
    )
    return await logs_crud.get_admin_logs(
        db,
        log_filter=log_filter,
        skip=skip,
        limit=limit
    )

@router.get("/statistics")
async def get_log_statistics(
    start_date: Optional[datetime] = None,
    end_date: Optional[datetime] = None,
    db: AsyncSession = Depends(get_session),
    _: str = Depends(check_admin_role(["admin", "manager"]))
):
    return await logs_crud.get_log_statistics(db, start_date, end_date)