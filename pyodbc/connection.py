
import _pyodbc

class Cursor:
    def __init__(self, cnxn, hstmt):
        self.cnxn = cnxn
        self.hstmt = hstmt

    def __repr__(self):
        return f'<Cursor id={id(self)} cnxn={id(self.cnxn)} hstmt={self.hstmt}>'


class Connection:
    def __init__(self, hdbc, settings):
        self.hdbc = hdbc
        self.settings = settings  # read-only settings from the factory

    def __repr__(self):
        return f'<Connection id={id(self)} hdbc={self.hdbc}>'
