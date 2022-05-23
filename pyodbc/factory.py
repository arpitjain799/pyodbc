
from .connection import Connection
import _pyodbc

class Settings:
    # Should this be the factory itself?
    def __init__(self, *, searchescape=None, supports_describeparam=True, datetime_precision=12):
        self.searchescape = searchescape
        self.supports_describeparam = supports_describeparam
        self.datetime_precision = datetime_precision
        #  self.maxwrite = maxwrite

        #  self.varchar_maxlength = None
        #  self.wvarchar_maxlength = None
        #  self.binary_maxlength = None

        self.need_long_data_len = True

        self.meta_enc = 'utf-8'


class Factory:
    """
    A connection factory, allowing configuration to be cached.
    """
    def __init__(self, cstring, pool=True):
        self.cstring = cstring
        self.settings = Settings()

        self.henv = _pyodbc.alloc_env(pool=pool)
        print('\n\n***** HENV:', self.henv)

    def connect(self) -> Connection:

        # REVIEW: Where should the encoding go?  If it is UTF-8, we don't want to allocate a
        # bunch of extra byte buffers just for this, do we?  Actually, for connecting it might
        # not be too bad, but it certainly would be for parameters.  (Or would it?)

        cstring = self.cstring.encode(self.settings.meta_enc)

        hdbc = _pyodbc.connect(henv, cstring)
