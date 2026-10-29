from sqlalchemy import Column, Integer, String, ForeignKey, Boolean, Text, DateTime
from sqlalchemy.orm import relationship
from ..db.base import Base
from datetime import datetime, timezone

class Device(Base):
    __tablename__ = 'devices'

    id = Column(Integer, primary_key=True, index=True)
    mac_address = Column(String(50), unique=True, nullable=False)
    current_firmware_version = Column(String(50), nullable=False)
    batch_id = Column(Integer, ForeignKey('batches.id'))
    status = Column(String(50), nullable=False)
    last_check_time = Column(DateTime, default=datetime.now(tz=timezone.utc))
    last_update_time = Column(DateTime, default=datetime.now(tz=timezone.utc))
    created_at = Column(DateTime, default=datetime.now(tz=timezone.utc))
    updated_at = Column(DateTime, default=datetime.now(tz=timezone.utc))
