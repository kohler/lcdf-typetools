#ifndef FINDMET_HH
#define FINDMET_HH
#include "hashmap.hh"
#include "vector.hh"
#include "permstr.hh"
class Filename;
class Metrics;
class AmfmMetrics;
class ErrorHandler;
class PsresDatabase;


class MetricsFinder {
  
  MetricsFinder *_next;
  MetricsFinder *_prev;
  
  MetricsFinder(const MetricsFinder &)			{ }
  MetricsFinder &operator=(const MetricsFinder &)	{ return *this; }

 protected:
  
  Metrics *try_metrics_file(const Filename &, MetricsFinder *, ErrorHandler *);
  AmfmMetrics *try_amfm_file(const Filename &, MetricsFinder *, ErrorHandler*);
  
 public:
  
  MetricsFinder();
  virtual ~MetricsFinder();

  MetricsFinder *next() const			{ return _next; }
  
  void add_finder(MetricsFinder *);
  
  Metrics *find_metrics(PermString, ErrorHandler * = 0);
  AmfmMetrics *find_amfm(PermString, ErrorHandler * = 0);
  
  virtual Metrics *find_metrics_x(PermString, MetricsFinder *, ErrorHandler *);
  virtual AmfmMetrics *find_amfm_x(PermString, MetricsFinder *, ErrorHandler*);
  
  void record(Metrics *m);
  virtual void record(Metrics *, PermString);
  virtual void record(AmfmMetrics *);
  
};


class CacheMetricsFinder: public MetricsFinder {
  
  HashMap<PermString, int> _metrics_map;
  Vector<Metrics *> _metrics;
  HashMap<PermString, int> _amfm_map;
  Vector<AmfmMetrics *> _amfm;
  
 public:
  
  CacheMetricsFinder();
  ~CacheMetricsFinder();
  
  Metrics *find_metrics_x(PermString, MetricsFinder *, ErrorHandler *);
  AmfmMetrics *find_amfm_x(PermString, MetricsFinder *, ErrorHandler *);
  void record(Metrics *, PermString);
  void record(AmfmMetrics *);

  void clear();
  
};


class InstanceMetricsFinder: public MetricsFinder {

  bool _call_mmpfb;
  
  Metrics *find_metrics_instance(PermString, MetricsFinder *, ErrorHandler *);
  
 public:
  
  InstanceMetricsFinder(bool call_mmpfb = true);
  
  Metrics *find_metrics_x(PermString, MetricsFinder *, ErrorHandler *);
  
};


class PsresMetricsFinder: public MetricsFinder {

  PsresDatabase *_psres;
  
 public:
  
  PsresMetricsFinder(PsresDatabase *);
  
  Metrics *find_metrics_x(PermString, MetricsFinder *, ErrorHandler *);
  AmfmMetrics *find_amfm_x(PermString, MetricsFinder *, ErrorHandler *);
  
};


class DirectoryMetricsFinder: public MetricsFinder {
  
  PermString _directory;
  
 public:
  
  DirectoryMetricsFinder(PermString);
  
  Metrics *find_metrics_x(PermString, MetricsFinder *, ErrorHandler *);
  AmfmMetrics *find_amfm_x(PermString, MetricsFinder *, ErrorHandler *);
  
};

#endif
