import warnings
from hakoniwa.hakopy import *

warnings.warn(
    "'import hakopy' is deprecated and will be removed in a future version. "
    "Please use 'from hakoniwa import hakopy' instead.",
    DeprecationWarning,
    stacklevel=2
)
