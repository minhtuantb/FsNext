#!/usr/bin/env python3
"""Encode a new Fshare app_key into the XOR-obfuscated C byte array used
by FshareApi::getAppKey().

The XOR key is the LAST byte of the array (0x5c by default). This matches
the XOR obfuscation scheme used by FshareApi::getAppKey(). Not cryptographic — just keeps
the plaintext key out of a naive `strings` dump of the binary.

Usage:
    python encode_appkey.py <new_key>
    python encode_appkey.py dMnqMMZMUnN5YpvKENaEhdQQ5jxDqddt

Paste the output into FshareApi.cpp::getAppKey() to replace the array.
"""
import sys

XOR_BYTE = 0x5c  # Match legacy client

def encode(key: str) -> list[int]:
    return [ord(c) ^ XOR_BYTE for c in key]

def main():
    if len(sys.argv) != 2:
        print("Usage: encode_appkey.py <plain_key>", file=sys.stderr)
        sys.exit(1)

    key = sys.argv[1]
    encoded = encode(key)

    print("    const char encrypted_key[] = {")
    for i in range(0, len(encoded), 10):
        chunk = encoded[i:i+10]
        print("        " + ", ".join(f"0x{b:02x}" for b in chunk) + ",")
    print(f"        0x{XOR_BYTE:02x}")
    print("    };")
    print()
    print(f"// Length: {len(key)} chars")

    # Round-trip verify
    decoded = "".join(chr(b ^ XOR_BYTE) for b in encoded)
    assert decoded == key, "Round-trip mismatch"
    print(f"// Decoded round-trip OK: {decoded}")

if __name__ == "__main__":
    main()
