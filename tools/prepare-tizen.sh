# $1 corresponds to gsignond-<ver>.tar.gz 
# $2 is the destination folder
# NOTE: all the files will be extracted under destination folder (instead of destfolder/gsignond-<version>)

if [ $# -ne 2 -o -z "$1" -o -z "$2" ]; then
    echo "Invalid arguments supplied"
    echo "Usage: ./prepare-tizen.sh <archive> <destination>"
    echo "NOTE: All the files will be extracted under destfolder (instead of destfolder/gsignond-<version>)"
    exit
fi

mkdir -p $2 && \
cd $2 && \
git rm -r *; rm -rf packaging;
tar -xzvf $1 -C $2 --strip-components 1 && \
mkdir -p packaging && \
cp -f dists/rpm/gsignond-tizen.spec packaging/gsignond.spec && \
cp -f dists/rpm/gsignond-tizen.changes packaging/gsignond.changes && \
cp -f dists/rpm/gsignond-tizen.manifest packaging/gsignond.manifest && \
git add -f *;
