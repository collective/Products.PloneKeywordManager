def to_str(s):
    if isinstance(s, bytes):
        s = s.decode("utf-8")
    return s
