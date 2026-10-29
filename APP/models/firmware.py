from sqlalchemy import Column, Integer, String, ForeignKey, Boolean, Text, DateTime
from sqlalchemy.orm import relationship
from ..db.base import Base
from datetime import datetime, timezone

class Firmware(Base):
    __tablename__ = 'firmware'
    
    id = Column(Integer, primary_key=True, index=True)
    version = Column(String(50), unique=True, nullable=False)
    file_path = Column(String(255), nullable=False)
    update_name = Column(String(255), nullable=False)
    release_notes = Column(Text, nullable=True)
    is_beta = Column(Boolean, default=False)
    file_size = Column(Integer, nullable=False)
    checksum = Column(String(32), nullable=False)  # MD5 hash is 32 characters
    created_at = Column(DateTime, default=datetime.now(tz=timezone.utc))
    updated_at = Column(DateTime, default=datetime.now(tz=timezone.utc))

class BatchFirmware(Base):
    __tablename__ = 'batch_firmware'

    batch_id = Column(Integer, ForeignKey('batches.id'), primary_key=True)
    firmware_id = Column(Integer, ForeignKey('firmware.id'), primary_key=True)
    is_active = Column(Boolean, default=True)
    created_at = Column(DateTime, default=datetime.now(tz=timezone.utc)) 