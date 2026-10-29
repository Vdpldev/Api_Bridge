from sqlalchemy.ext.asyncio import create_async_engine, AsyncSession, async_sessionmaker
from sqlalchemy.orm import sessionmaker
from typing import AsyncGenerator
from APP.core.config import settings
import logging

logger = logging.getLogger(__name__)

# Convert database URL to async if needed
def make_async_database_url(url: str) -> str:
    """Convert sync database URL to async"""
    if url.startswith("postgresql://"):
        return url.replace("postgresql://", "postgresql+asyncpg://")
    elif url.startswith("mysql://"):
        return url.replace("mysql://", "mysql+aiomysql://")
    elif url.startswith("sqlite://"):
        return url.replace("sqlite://", "sqlite+aiosqlite://")
    return url

ASYNC_DATABASE_URL = make_async_database_url(settings.DATABASE_URL)

# Create async engine
engine = create_async_engine(
    ASYNC_DATABASE_URL,
    echo=settings.DEBUG,  # Only echo SQL in debug mode
    pool_pre_ping=True,  # Verify connections before using
    pool_size=10,  # TODO: Adjust for production based on load
    max_overflow=20,  # TODO: Adjust for production
)

# Create async session factory
AsyncSessionLocal = async_sessionmaker(
    engine,
    class_=AsyncSession,
    expire_on_commit=False,
    autocommit=False,
    autoflush=False,
)

async def get_session() -> AsyncGenerator[AsyncSession, None]:
    """
    Dependency to get database session.
    Ensures proper transaction handling and cleanup.
    """
    async with AsyncSessionLocal() as session:
        try:
            yield session
            await session.commit()
        except Exception:
            await session.rollback()
            raise
        finally:
            await session.close()

async def init_db():
    """Initialize database tables"""
    try:
        # Import all models to ensure they're registered
        from APP.models import admin, batch, devices, firmware, logs
        from APP.db.base import Base
        
        async with engine.begin() as conn:
            # Create all tables
            await conn.run_sync(Base.metadata.create_all)
            
        logger.info("Database tables created successfully")
    except Exception as e:
        logger.error(f"Error creating database tables: {e}")
        raise

async def close_db():
    """Close database connections"""
    await engine.dispose()
    logger.info("Database connections closed")

# TODO: For production, consider:
# 1. Connection pooling configuration based on expected load
# 2. Read replica support for scaling read operations
# 3. Connection retry logic for network issues
# 4. Health check queries to verify database connectivity
# 5. Monitoring of connection pool metrics