#ifndef FINDMET_HH
#define FINDMET_HH
#ifdef __GNUG__
#pragma interface
#endif
#include "hashmap.hh"
#include "vector.hh"
#include "permstr.hh"
class Filename;
class Metrics;
class AmfmMetrics;
class ErrorHandler;


class MetricsFinder {
  
  MetricsFinder *_next;
  MetricsFinder *_prev;
  
  MetricsFinder(const MetricsFinder &)			{ }
  MetricsFinder &operator=(const MetricsFinder &)	{ return *this; }
  
 public:
  
  MetricsFinder();
  virtual ~MetricsFinder();

  MetricsFinder *next() const			{ return _next; }
  
  void append(MetricsFinder *);
  
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

  Metrics *find_metrics_instance(PermString, MetricsFinder *, ErrorHandler *);
  
 public:
  
  InstanceMetricsFinder();
  
  Metrics *find_metrics_x(PermString, MetricsFinder *, ErrorHandler *);
  
};


class PsresMetricsFinder: public MetricsFinder {
  
  HashMap<PermString, PermString> _afm_path_map;
  HashMap<PermString, PermString> _amfm_path_map;
  
 public:
  
  PsresMetricsFinder();
  void read_psres(const Filename &);
  
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
