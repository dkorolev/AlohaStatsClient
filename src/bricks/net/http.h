#include "../port.h"

#if defined(BRICKS_POSIX) || defined(BRICKS_APPLE) || defined(BRICKS_ANDROID)
#include "http/http.h"
#else
#error "No HTTP implementation available for your platform."
#endif
