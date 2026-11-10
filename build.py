#!/usr/bin/env python3
"""Build and package the ASI without requiring a game installation."""
from pathlib import Path
import hashlib,json,shutil,subprocess,sys,zipfile
ROOT=Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT/'tools'))
from patches import header

def main():
    build=ROOT/'build';build.mkdir(exist_ok=True)
    (build/'patches.h').write_text(header())
    scripts=ROOT/'dist/scripts';scripts.mkdir(parents=True,exist_ok=True)
    asi=scripts/'RE0.UISpeedTweaks.asi'
    subprocess.run(['i686-w64-mingw32-gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-shared','-static-libgcc','-Wl,--no-insert-timestamp','-Wl,--image-base,0x63ac0000','-I'+str(build),str(ROOT/'src/ui_speed_tweaks.c'),'-o',str(asi)],check=True)
    shutil.copy2(ROOT/'RE0.UISpeedTweaks.ini',scripts/'RE0.UISpeedTweaks.ini')
    archive=ROOT/'dist/RE0-UI-Speed-Tweaks-1.1.0.zip'
    with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
        for path in [asi,scripts/'RE0.UISpeedTweaks.ini']:
            z.write(path,'scripts/'+path.name)
        for name in ['README.md','README.ko.md','LICENSE']:
            z.write(ROOT/name,name)
    report={'asi_sha256':hashlib.sha256(asi.read_bytes()).hexdigest(),'package_sha256':hashlib.sha256(archive.read_bytes()).hexdigest()}
    (build/'build-report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(archive)
if __name__=='__main__':
    main()
