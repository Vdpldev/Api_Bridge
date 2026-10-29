"""remove duplicate user_data table

Revision ID: 003
Revises: 002

The diagram contains both ``user_data`` and ``users``. They represent the
same normal-user account concept, so ``users`` is retained as the canonical
table and the duplicate ``user_data`` table is removed.
"""
from typing import Sequence, Union

from alembic import op
import sqlalchemy as sa


revision: str = "003"
down_revision: Union[str, None] = "002"
branch_labels: Union[str, Sequence[str], None] = None
depends_on: Union[str, Sequence[str], None] = None


def upgrade() -> None:
    """Remove the duplicate normal-user table from the local schema."""
    op.drop_table("user_data")


def downgrade() -> None:
    """Restore the duplicate table for migration rollback compatibility."""
    op.create_table(
        "user_data",
        sa.Column("id", sa.Integer(), primary_key=True),
        sa.Column("username", sa.String(80), nullable=False),
        sa.Column("email", sa.String(120), nullable=False),
        sa.Column("password", sa.String(250), nullable=False),
        sa.Column("user_role", sa.String(200), nullable=True),
        sa.Column("remember_me", sa.Boolean(), nullable=True),
        sa.Column("verified", sa.Boolean(), nullable=True),
    )
