# Initial install on a fresh system of core libraries and tools

sudo apt-get update
sudo apt-get -y install g++ cmake
sudo apt-get -y install libglew-dev
sudo apt-get -y install mesa-common-dev
sudo apt-get -y install libopenal-dev
sudo apt-get -y install libalut-dev
sudo apt-get -y install build-essential
sudo apt-get -y install libcurl4-openssl-dev
sudo apt-get -y install ruby
sudo apt-get -y install libx11-dev

# Creating build directories in a fresh application folder.

mkdir Release
cd Release
cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release

# For Debug:

cd ..
mkdir Debug
cd Debug
cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug


# Building (from within a Release or Debug directory)

make -j6 install