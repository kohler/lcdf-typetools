#ifndef MYFONT_HH
#define MYFONT_HH
#include <efont/t1font.hh>
class Efont::EfontMMSpace;
class ErrorHandler;

class MyFont: public Efont::Type1Font { public:
  
    MyFont(Efont::Type1Reader &);
    ~MyFont();
  
    bool set_design_vector(Efont::EfontMMSpace *, const Vector<double> &,
			   ErrorHandler * = 0);
  
    void interpolate_dicts(ErrorHandler * = 0);
    void interpolate_charstrings(int precision, ErrorHandler * = 0);

  private:
  
    typedef Vector<double> NumVector;
  
    int _nmasters;
    Vector<double> _weight_vector;
  
    void interpolate_dict_int(PermString, ErrorHandler *, Dict = dPrivate);
    void interpolate_dict_num(PermString, Dict = dPrivate);
    void interpolate_dict_numvec(PermString, Dict = dPrivate, bool = false);
    void kill_def(Efont::Type1Definition *, int which_dict = -1); 
  
};

#endif
