Fresh


(c) 2012 Jeff Wofford

biz@jeffwofford.com

www.jeffwofford.com 


=====



Build instructions: Win32



Required Tools: MingW, MSYS, and CMake.

Required Libraries: OpenGL, OpenAL.

	mkdir build
	
	cd build
	set "OPENALDIR=C:\Program Files (x86)\OpenAL 1.1 SDK"	# Or wherever OpenAL is installed on your system
	cmake -G "MinGW Makefiles" ..
	
	mingw32-make
