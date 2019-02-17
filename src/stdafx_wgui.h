#include "wgui/wibase.h"
#include "wgui/wihandle.h"
#include "materialmanager.h"
#include <wrappers/command_buffer.h> // Has to be included before cmaterialmanager.h! (Otherwise there will be strange naming conflicts in VS2017.)
#include "cmaterialmanager.h"
#include <unordered_map>
#include <algorithm>
#include <string>
#include "wgui/wiincludes.h"
#include <algorithm>
#include <deque>