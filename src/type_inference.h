#pragma once
#include "type_system.h"

class IAST;

void infer_types(std::shared_ptr<IAST>& root);
