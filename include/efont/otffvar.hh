// -*- related-file-name: "../../libefont/otffvar.cc" -*-
#ifndef EFONT_OTFFVAR_HH
#define EFONT_OTFFVAR_HH
#include <efont/otf.hh>
#include <lcdf/error.hh>
#include <utility>
namespace Efont { namespace OpenType {
class Axis;

class Fvar { public:
    Fvar(const Data&);
    // default destructor

    inline int naxes() const;
    inline Axis axis(int i) const;

  private:
    Data _d;

    enum { HEADER_SIZE = 16, AXIS_SIZE = 20,
           X_AXISOFF = 4, X_AXISCOUNT = 8, X_AXISSIZE = 10 };

};

class Axis { public:
    inline Axis(const unsigned char* d) : _d(d) {}

    inline Tag tag() const;
    inline double min_value() const;
    inline double default_value() const;
    inline double max_value() const;
    inline uint16_t flags() const;
    inline int nameid() const;

  private:
    const unsigned char* _d;
};

inline Tag Axis::tag() const {
    return Tag((Data::u16_aligned(_d) << 16) | Data::u16_aligned(_d + 2));
}

inline double Axis::min_value() const {
    return Data::fixed_aligned(_d + 4);
}

inline double Axis::default_value() const {
    return Data::fixed_aligned(_d + 8);
}

inline double Axis::max_value() const {
    return Data::fixed_aligned(_d + 12);
}

inline uint16_t Axis::flags() const {
    return Data::u16_aligned(_d + 16);
}

inline int Axis::nameid() const {
    return Data::u16_aligned(_d + 18);
}


inline int Fvar::naxes() const{
    return _d.u16(X_AXISCOUNT);
}

inline Axis Fvar::axis(int i) const {
    assert(i >= 0 && i < naxes());
    return Axis(_d.udata() + _d.u16(X_AXISOFF) + i * _d.u16(X_AXISSIZE));
}

}}
#endif
