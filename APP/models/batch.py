from sqlalchemy import Column, Integer, String, ForeignKey, Boolean, Text, DateTime, Enum as SQLAlchemyEnum
from sqlalchemy.orm import relationship
from ..db.base import Base
from datetime import datetime, timezone
from enum import Enum

class BatchUpdateState(str, Enum):
    IDLE = "IDLE"
    UPDATING = "UPDATING"
    COMPLETED = "COMPLETED"
    FAILED = "FAILED"

class Batch(Base):
    __tablename__ = 'batches'

    id = Column(Integer, primary_key=True, index=True)
    name = Column(String(255), nullable=False)
    description = Column(Text, nullable=True)
    check_update_time = Column(DateTime, nullable=False)
    auto_update = Column(Boolean, default=False)
    update_state = Column(SQLAlchemyEnum(BatchUpdateState), default=BatchUpdateState.IDLE)
    last_update_check = Column(DateTime, nullable=True)
    last_update_trigger = Column(DateTime, nullable=True)
    devices_total = Column(Integer, default=0)
    devices_updated = Column(Integer, default=0)
    devices_failed = Column(Integer, default=0)
    created_at = Column(DateTime, default=datetime.now(tz=timezone.utc))
    updated_at = Column(DateTime, default=datetime.now(tz=timezone.utc), onupdate=datetime.now(tz=timezone.utc))

