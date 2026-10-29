"""change the enum type

Revision ID: e429538b831a
Revises: 
Create Date: 2025-08-01 11:32:58.051214

"""
from typing import Sequence, Union

from alembic import op
import sqlalchemy as sa
from sqlalchemy.dialects import mysql

# revision identifiers, used by Alembic.
revision: str = 'e429538b831a'
down_revision: Union[str, None] = None
branch_labels: Union[str, Sequence[str], None] = None
depends_on: Union[str, Sequence[str], None] = None


def upgrade() -> None:
    """
    This function is executed when you run 'alembic upgrade head'.
    It changes the 'level' column in both tables from a native ENUM
    to a VARCHAR(20) to match the 'native_enum=False' setting in the models.
    """
    # ### Commands to upgrade the database schema ###

    # --- Alter 'admin_logs' table ---
    op.alter_column('admin_logs', 'level',
               # This is the old type that Alembic expects to find in the database
               existing_type=mysql.ENUM('DEBUG', 'INFO', 'WARNING', 'ERROR', name='loglevel'),
               # This is the new type we are changing it to
               type_=sa.String(length=20),
               # This ensures the NOT NULL constraint is kept
               existing_nullable=False)

    # --- Alter 'device_update_logs' table ---
    op.alter_column('device_update_logs', 'level',
               existing_type=mysql.ENUM('DEBUG', 'INFO', 'WARNING', 'ERROR', name='loglevel'),
               type_=sa.String(length=20),
               existing_nullable=False)

    # ### end Alembic commands ###


def downgrade() -> None:
    """
    This function is executed when you run 'alembic downgrade ...'.
    It reverts the changes made in the upgrade() function, changing
    the columns back to a native ENUM type.
    """
    # ### Commands to downgrade the database schema ###

    # --- Revert 'device_update_logs' table ---
    op.alter_column('device_update_logs', 'level',
               # This is the type before downgrading (the new state)
               existing_type=sa.String(length=20),
               # This is the type we are reverting back to (the old state)
               type_=mysql.ENUM('DEBUG', 'INFO', 'WARNING', 'ERROR', name='loglevel'),
               existing_nullable=False)

    # --- Revert 'admin_logs' table ---
    op.alter_column('admin_logs', 'level',
               existing_type=sa.String(length=20),
               type_=mysql.ENUM('DEBUG', 'INFO', 'WARNING', 'ERROR', name='loglevel'),
               existing_nullable=False)

    # ### end Alembic commands ###
