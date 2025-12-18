import os
import sys
import shutil
import zipfile
from glob import glob


def extract(package_name, local_dir):
    os.makedirs(local_dir, exist_ok=True)
    os.chdir(local_dir)

    pkgdir = sys.modules[package_name].__path__[0]
    # extract all *.zip files in the package, preserve relative file paths
    for fullpath in glob(os.path.join(pkgdir, "**/*.zip"), recursive=True):
        # determine zip relative filepath
        rel_filepath = os.path.relpath(fullpath, pkgdir)
        rel_dir = os.path.dirname(rel_filepath)
        if rel_dir and len(rel_dir) > 0:
            os.makedirs(rel_dir, exist_ok=True)
        # copy archive under current dir
        temp_zip_path = os.path.join(os.getcwd(), rel_filepath)
        shutil.copy(fullpath, temp_zip_path)
        with zipfile.ZipFile(rel_filepath, 'r') as zip_ref:
            zip_ref.extractall(rel_dir)
        os.unlink(temp_zip_path)

    os.chdir("..")


def setup(local_dir="stash"):
    """
        Extract binary and schema into a local directory.
    """
    extract('emod_malaria', local_dir)


if __name__ == "__main__":
    setup()
