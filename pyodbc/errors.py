"""
Exceptions raised by this package.  The hierarchy is defined by the DB API.
"""

class Error(Exception):
    def __init__(self, message, sqlstate=None):
        Exception.__init__(self, message)
        self.sqlstate = sqlstate


class IntegrityError(Error):
    pass
