import os
import zipfile

PROJECT_ROOT = r"C:\Users\Liang Zhou\Documents\School Stuff\Trimester 2\csd1451\repos\EBGDW"
RUBRICS_SRC = r"C:\Users\Liang Zhou\Downloads\EarlyBirdGetsDaWorm_rubrics.xlsm"
# Update path above if the file was renamed
BIN_DIR = os.path.join(PROJECT_ROOT, "bin", "Release-x64")
OUTPUT_ZIP = os.path.join(PROJECT_ROOT, "EarlyBirdGetsDaWorm_EchoesOfTheWard.zip")

EXCLUDE_DIRS = {".vs", ".tmp", "bin", "Debug", "Release", ".git", ".svn", ".claude", ".github"}
EXCLUDE_EXTS = {".ipch", ".suo", ".db", ".pdb", ".exe", ".ilk", ".tlog", ".obj",
                ".idb", ".log", ".exp", ".iobj", ".ipdb", ".sdf", ".pch"}
EXCLUDE_FILES = {"package_submission.py", ".gitignore", ".gitattributes", "EarlyBirdGetsDaWorm_EchoesOfTheWard.zip"}
PRODUCT_EXCLUDE_EXTS = {".pdb", ".ilk", ".exp", ".lib"}

def should_skip(dirpath, name):
    if name in EXCLUDE_DIRS or name in EXCLUDE_FILES:
        return True
    _, ext = os.path.splitext(name)
    return ext.lower() in EXCLUDE_EXTS

with zipfile.ZipFile(OUTPUT_ZIP, 'w', zipfile.ZIP_DEFLATED) as zf:
    # SOURCE
    for dirpath, dirnames, filenames in os.walk(PROJECT_ROOT):
        dirnames[:] = [d for d in dirnames if not should_skip(dirpath, d)]
        for f in filenames:
            if should_skip(dirpath, f):
                continue
            full = os.path.join(dirpath, f)
            zf.write(full, os.path.join("SOURCE", os.path.relpath(full, PROJECT_ROOT)))

    # DOCUMENTS
    zf.write(RUBRICS_SRC, os.path.join("DOCUMENTS", "EarlyBirdGetsDaWorm_rubrics.xlsm"))

    # PRODUCT
    for dirpath, dirnames, filenames in os.walk(BIN_DIR):
        for f in filenames:
            _, ext = os.path.splitext(f)
            if ext.lower() in PRODUCT_EXCLUDE_EXTS:
                continue
            full = os.path.join(dirpath, f)
            zf.write(full, os.path.join("PRODUCT", os.path.relpath(full, BIN_DIR)))

size_mb = os.path.getsize(OUTPUT_ZIP) / (1024 * 1024)
print(f"Created: {OUTPUT_ZIP}")
print(f"Size: {size_mb:.2f} MB")
