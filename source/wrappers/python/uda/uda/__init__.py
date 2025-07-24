print("NOTE: this package has been renamed as \"pyuda\". The uda package name is now just a wrapper around \"pyuda\"; uda will be deprecated.")
import warnings
warnings.warn("The \"uda\" package name is deprecated. Please use \"pyuda\" when pip installing the package or importing into python", DeprecationWarning)

import pyuda as _pyuda

# Copy public attributes into the oldname namespace
globals().update({
    name: getattr(_pyuda, name)
    for name in getattr(_pyuda, '__all__', dir(_pyuda))
    if not name.startswith('_')
})

# Optionally set __all__ to the same as newname
__all__ = getattr(_pyuda, '__all__', [])
