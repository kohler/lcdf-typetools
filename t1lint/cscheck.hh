#ifndef CSCHECK_HH
#define CSCHECK_HH
#include "t1interp.hh"
#include "point.hh"
#include "vector.hh"
class ErrorHandler;

class CharstringChecker : public CharstringInterp { public:
  
    CharstringChecker(EfontProgram *, Vector<double> *);
  
    void init();
    bool error(int, int);
    bool callothersubr();
    bool type1_command(int);
  
    bool check(Charstring &, ErrorHandler *);
  
  private:
    
    ErrorHandler *_errh;

    point _cp;
  
    bool _started;
    bool _flex;
    bool _cp_exists;
  
    bool _hstem;
    bool _hstem3;
    bool _vstem;
    bool _vstem3;

    Vector<double> _h_hstem;
    Vector<double> _h_vstem;

    Vector<double> _h_hstem3;
    Vector<double> _h_vstem3;

    void stem(double, double, const char *);
    void check_stem3(const char *);
  
    void moveto(double, double, bool cp_exists = true);
    void rmoveto(double, double);
    void rlineto(double, double);
    void rrcurveto(double, double, double, double, double, double);
  
};

#endif
