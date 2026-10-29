from sqlalchemy import Column, Integer, String, ForeignKey, Boolean, Text, DateTime, Enum
from sqlalchemy.orm import relationship
from ..db.base import Base
from datetime import datetime, timezone
from ..schemas.logs import LogLevel

class DeviceUpdateLog(Base):
    __tablename__ = 'device_update_logs'

    id = Column(Integer, primary_key=True, index=True)
    device_id = Column(Integer, ForeignKey('devices.id'))
    firmware_id = Column(Integer, ForeignKey('firmware.id'))
    status = Column(String(50), nullable=False)
    error_message = Column(Text, nullable=True)
    level = Column(Enum(LogLevel, native_enum=False), nullable=False, default=LogLevel.INFO)
    created_at = Column(DateTime, default=datetime.now(tz=timezone.utc))
    updated_at = Column(DateTime, default=datetime.now(tz=timezone.utc), onupdate=datetime.now(tz=timezone.utc))

class AdminLog(Base):
    __tablename__ = 'admin_logs'

    id = Column(Integer, primary_key=True, index=True)
    admin_id = Column(Integer, ForeignKey('admin_users.id'), nullable=False)
    action_type = Column(String(50), nullable=False)
    entity_type = Column(String(50), nullable=False)
    entity_id = Column(Integer, nullable=False)
    details = Column(Text, nullable=True)
    level = Column(Enum(LogLevel, native_enum=False), nullable=False, default=LogLevel.INFO)
    created_at = Column(DateTime, default=datetime.now(tz=timezone.utc))