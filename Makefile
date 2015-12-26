default: relclean relmake

relmake: tFBE tFBShell tFBV tFBTools

tFBE:
	@cd FBE
	$(MAKE)
	@cd ..

tFBShell:
	@cd FBShell
	$(MAKE) /f FBShell.mak CFG="FBShell - Win32 Release"
	@cd ..

tFBV:
	@cd FBV
	$(MAKE) /f FBV.mak CFG="FBV - Win32 Release"
	@cd ..

tFBTools:
	@cd FBTools
	"C:\Program Files\NSIS\makensis.exe" /V2 fbtools.nsi
	@cd ..

relclean:
	@cd FBE
	$(MAKE) clean
	@cd ..
	@cd FBShell
	$(MAKE) /f FBShell.mak CFG="FBShell - Win32 Release" clean
	@cd ..
	@cd FBV
	$(MAKE) /f FBV.mak CFG="FBV - Win32 Release" clean
	@cd ..

