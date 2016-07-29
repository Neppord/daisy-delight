from distutils.core import setup, Extension
import platform
if platform.system() == 'Darwin':
	extmodule =[Extension(
  "audio",sources=["src/audiomodule.c"] ,extra_link_args=['-framework', 'AudioUnit','-framework', 'CoreServices','-framework','AudioToolbox'])]
else:
	extmodule = []
setup(name= "Daisy Delight",
    version="0.1a",
    description="A easy to use Daisy player for the open world!", 
    package_dir = {'': 'src'},
    packages=["daisyDelight"],
    ext_package='daisyDelight',
    ext_modules=extmodule,
    scripts=["src/scripts/daisyDelight"]

    )
