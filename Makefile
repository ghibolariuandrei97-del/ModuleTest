.RECIPEPREFIX = >
CXX ?= g++
OUT = mymodule_amxx_i386.so
INC = -Isdk \
 -Ideps/amxmodx/public -Ideps/amxmodx/public/amtl \
 -Ideps/hlsdk/common -Ideps/hlsdk/dlls -Ideps/hlsdk/engine \
 -Ideps/hlsdk/pm_shared -Ideps/hlsdk/public \
 -Ideps/metamod-hl1/metamod
CXXFLAGS = -m32 -fPIC -O2 -std=gnu++11 -fno-rtti -fno-exceptions -w -DLINUX

all: $(OUT)

$(OUT): sdk/amxxmodule.cpp hudsprites.cpp
> $(CXX) $(CXXFLAGS) $(INC) -shared -static-libgcc -static-libstdc++ -o $@ $^ -ldl

clean:
> rm -f $(OUT)
