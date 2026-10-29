import pytest

from APP.core.security import get_password_hash, verify_password


def test_verify_password_long_input_returns_false():
    hashed = get_password_hash("correct-password")

    result = verify_password("a" * 200, hashed)

    assert result is False


def test_get_password_hash_rejects_too_long_password():
    with pytest.raises(ValueError):
        get_password_hash("a" * 200)
