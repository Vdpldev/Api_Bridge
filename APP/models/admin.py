from sqlalchemy import Column, Integer, String, Enum, DateTime, Boolean
from APP.db.base import Base
from datetime import datetime, timezone

class AdminUser(Base):
    __tablename__ = 'admin_users'

    id = Column(Integer, primary_key=True, index=True)
    username = Column(String(255), unique=False, nullable=False)  # Set length to 255
    email = Column(String(255), unique=True, nullable=False)  # Set length to 255
    hashed_password = Column(String(255), nullable=False)  # Set length to 255
    role = Column(Enum('admin', 'manager', 'viewer', name='user_roles'), nullable=False)
    is_active = Column(Boolean, default=True)
    last_login = Column(DateTime, nullable=True)
    created_at = Column(DateTime, default=datetime.now(tz=timezone.utc))
    updated_at = Column(DateTime, default=datetime.now(tz=timezone.utc), onupdate=datetime.now(tz=timezone.utc))