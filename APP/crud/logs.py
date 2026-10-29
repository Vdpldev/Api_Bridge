from sqlalchemy.ext.asyncio import AsyncSession
from sqlalchemy import select, and_
from typing import List, Optional
from datetime import datetime
from ..models.logs import DeviceUpdateLog, AdminLog
from ..schemas.logs import UpdateLogCreate, AdminLogCreate, LogFilter

async def create_update_log(
    db: AsyncSession, 
    log: UpdateLogCreate
) -> DeviceUpdateLog:
    db_log = DeviceUpdateLog(**log.model_dump())
    db.add(db_log)
    await db.flush()
    await db.refresh(db_log)
    return db_log

async def create_admin_log(
    db: AsyncSession,
    log: AdminLogCreate
) -> AdminLog:
    db_log = AdminLog(**log.model_dump())
    db.add(db_log)
    await db.flush()
    await db.refresh(db_log)
    return db_log

async def get_device_logs(
    db: AsyncSession,
    device_id: Optional[int] = None,
    log_filter: Optional[LogFilter] = None,
    skip: int = 0,
    limit: int = 100
) -> List[DeviceUpdateLog]:
    query = select(DeviceUpdateLog)
    
    if device_id is not None:
        query = query.filter(DeviceUpdateLog.device_id == device_id)
    
    if log_filter:
        if log_filter.start_date:
            query = query.filter(DeviceUpdateLog.created_at >= log_filter.start_date)
        if log_filter.end_date:
            query = query.filter(DeviceUpdateLog.created_at <= log_filter.end_date)
        if log_filter.level:
            query = query.filter(DeviceUpdateLog.level == log_filter.level)
    
    query = query.order_by(DeviceUpdateLog.created_at.desc()).offset(skip).limit(limit)
    result = await db.execute(query)
    return result.scalars().all()

async def get_admin_logs(
    db: AsyncSession,
    log_filter: Optional[LogFilter] = None,
    skip: int = 0,
    limit: int = 100
) -> List[AdminLog]:
    query = select(AdminLog)
    
    if log_filter:
        if log_filter.start_date:
            query = query.filter(AdminLog.created_at >= log_filter.start_date)
        if log_filter.end_date:
            query = query.filter(AdminLog.created_at <= log_filter.end_date)
        if log_filter.level:
            query = query.filter(AdminLog.level == log_filter.level)
        if log_filter.entity_type:
            query = query.filter(AdminLog.entity_type == log_filter.entity_type)
        if log_filter.entity_id:
            query = query.filter(AdminLog.entity_id == log_filter.entity_id)
        if log_filter.action_type:
            query = query.filter(AdminLog.action_type == log_filter.action_type)
    
    query = query.order_by(AdminLog.created_at.desc()).offset(skip).limit(limit)
    result = await db.execute(query)
    return result.scalars().all()

async def get_log_statistics(
    db: AsyncSession,
    start_date: Optional[datetime] = None,
    end_date: Optional[datetime] = None
):
    """Get statistics about logs in a given time period"""
    device_logs_query = select(DeviceUpdateLog)
    admin_logs_query = select(AdminLog)
    
    if start_date:
        device_logs_query = device_logs_query.filter(DeviceUpdateLog.created_at >= start_date)
        admin_logs_query = admin_logs_query.filter(AdminLog.created_at >= start_date)
    if end_date:
        device_logs_query = device_logs_query.filter(DeviceUpdateLog.created_at <= end_date)
        admin_logs_query = admin_logs_query.filter(AdminLog.created_at <= end_date)
    
    device_logs = await db.execute(device_logs_query)
    admin_logs = await db.execute(admin_logs_query)
    
    return {
        "device_logs_count": len(device_logs.scalars().all()),
        "admin_logs_count": len(admin_logs.scalars().all())
    }