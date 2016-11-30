#include "message.h"

std::ostream& operator<< (std::ostream& out, const Entry& e) {
    return out << "(" << e.id << "," << e.pid << ")";
}