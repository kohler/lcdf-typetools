#ifdef __GNUG__
#pragma implementation "findmet.hh"
#endif
#include "findmet.hh"
#include "afmparse.hh"
#include "afm.hh"
#include "amfm.hh"
#include <string.h>
#include <stdlib.h>


inline
MetricsFinder::MetricsFinder()
  : _next(0), _prev(0)
{
}

MetricsFinder::~MetricsFinder()
{
  if (_next) _next->_prev = _prev;
  if (_prev) _prev->_next = _next;
}

void
MetricsFinder::append(MetricsFinder *new_finder)
{
  if (_next)
    _next->append(new_finder);
  else {
    assert(!new_finder->_prev);
    new_finder->_prev = this;
    _next = new_finder;
  }
}

Metrics *
MetricsFinder::find_metrics(PermString name, ErrorHandler *errh)
{
  MetricsFinder *f = this;
  while (f) {
    Metrics *m = f->find_metrics_x(name, this, errh);
    if (m) return m;
    f = f->_next;
  }
  return 0;
}

AmfmMetrics *
MetricsFinder::find_amfm(PermString name, ErrorHandler *errh)
{
  MetricsFinder *f = this;
  while (f) {
    AmfmMetrics *m = f->find_amfm_x(name, this, errh);
    if (m) return m;
    f = f->_next;
  }
  return 0;
}

void
MetricsFinder::record(Metrics *m)
{
  record(m, m->font_name());
}

void
MetricsFinder::record(Metrics *m, PermString name)
{
  if (_next) _next->record(m, name);
}

void
MetricsFinder::record(AmfmMetrics *amfm)
{
  if (_next) _next->record(amfm);
}

Metrics *
MetricsFinder::find_metrics_x(PermString, MetricsFinder *, ErrorHandler *)
{
  return 0;
}

AmfmMetrics *
MetricsFinder::find_amfm_x(PermString, MetricsFinder *, ErrorHandler *)
{
  return 0;
}


/*****
 * CacheMetricsFinder
 **/

CacheMetricsFinder::CacheMetricsFinder()
  : _metrics_map(-1), _amfm_map(-1)
{
}

CacheMetricsFinder::~CacheMetricsFinder()
{
  clear();
}

Metrics *
CacheMetricsFinder::find_metrics_x(PermString name, MetricsFinder *,
				   ErrorHandler *)
{
  int index = _metrics_map[name];
  return index >= 0 ? _metrics[index] : 0;
}

AmfmMetrics *
CacheMetricsFinder::find_amfm_x(PermString name, MetricsFinder *,
				ErrorHandler*)
{
  int index = _amfm_map[name];
  return index >= 0 ? _amfm[index] : 0;
}


void
CacheMetricsFinder::record(Metrics *m, PermString name)
{
  int index = _metrics.append(m);
  _metrics_map.insert(name, index);
  m->use();
  MetricsFinder::record(m, name);
}

void
CacheMetricsFinder::record(AmfmMetrics *amfm)
{
  int index = _amfm.append(amfm);
  _amfm_map.insert(amfm->font_name(), index);
  amfm->use();
  MetricsFinder::record(amfm);
}

void
CacheMetricsFinder::clear()
{
  for (int i = 0; i < _metrics.count(); i++)
    _metrics[i]->unuse();
  for (int i = 0; i < _amfm.count(); i++)
    _amfm[i]->unuse();
  _metrics.clear();
  _amfm.clear();
  _metrics_map.clear();
  _amfm_map.clear();
}


/*****
 * InstanceMetricsFinder
 **/

InstanceMetricsFinder::InstanceMetricsFinder()
{
}

Metrics *
InstanceMetricsFinder::find_metrics_instance(PermString name,
				MetricsFinder *finder, ErrorHandler *errh)
{
  char *underscore = strchr(name, '_');
  PermString amfm_name =
    PermString(name.cc(), underscore - name.cc());
  
  AmfmMetrics *amfm = finder->find_amfm(amfm_name, errh);
  if (!amfm) return 0;
  
  Type1MMSpace *mmspace = amfm->mmspace();
  Vector<double> design = mmspace->default_design_vector();
  int i = 0;
  while (underscore[0] == '_' && underscore[1]) {
    double x = strtod(underscore + 1, &underscore);
    mmspace->set_design(design, i, x, errh);
    i++;
  }
  
  Vector<double> weight;
  if (!mmspace->design_to_weight(design, weight, errh))
    return 0;
  Metrics *new_afm = amfm->interpolate(design, weight, errh);
  if (new_afm) {
    finder->record(new_afm);
    // What if the dimensions changed because the user specified out-of-range
    // dimens? We don't want to reinterpolate each time, so record the new
    // AFM under that name as well.
    if (new_afm->font_name() != name)
      finder->record(new_afm, name);
  }
  return new_afm;
}

Metrics *
InstanceMetricsFinder::find_metrics_x(PermString name, MetricsFinder *finder,
				      ErrorHandler *errh)
{
  if (strchr(name, '_'))
    return find_metrics_instance(name, finder, errh);
  else
    return 0;
}


/*****
 * PsresMetricsFinder
 **/

PsresMetricsFinder::PsresMetricsFinder()
{
}

void
PsresMetricsFinder::read_psres(const Filename &file_name)
{
  Slurper slurper(file_name);
  AfmParser l(slurper);
  AfmParser::set_ends_names('=', true);
  
  PermString directory = file_name.directory();
  PermString font, file;
  
  while (l.next_line() && !l.is("."))
    ;
  
  while (l.next_line() && !l.is("FontAFM"))
    ;
  while (l.next_line() && !l.is("."))
    if (l.is("%/s=%+s", &font, &file) && !_afm_path_map[font]) {
      PermString path =
	permprintf("%p/%p", directory.capsule(), file.capsule());
      _afm_path_map.insert(font, path);
    }
  
  while (l.next_line() && !l.is("FontAMFM"))
    ;
  while (l.next_line() && !l.is("."))
    if (l.is("%/s=%+s", &font, &file) && !_amfm_path_map[font]) {
      PermString path =
	permprintf("%p/%p", directory.capsule(), file.capsule());
      _amfm_path_map.insert(font, path);
    }
  
  AfmParser::set_ends_names('=', false);
}

Metrics *
PsresMetricsFinder::find_metrics_x(PermString name, MetricsFinder *finder,
				   ErrorHandler *errh)
{
  PermString tryname = _afm_path_map[name];
  Filename newfile = tryname;
  if (newfile.readable()) {
    Slurper slurper(newfile);
    AfmReader reader(slurper, errh);
    Metrics *afm = reader.take();
    if (afm) finder->record(afm);
    return afm;
  }
  return 0;
}

AmfmMetrics *
PsresMetricsFinder::find_amfm_x(PermString name, MetricsFinder *finder,
				ErrorHandler *errh)
{
  PermString tryname = _amfm_path_map[name];
  Filename newfile = tryname;
  if (newfile.readable()) {
    Slurper slurper(newfile);
    AmfmReader reader(slurper, finder, errh);
    AmfmMetrics *amfm = reader.take();
    if (amfm) finder->record(amfm);
    return amfm;
  }
  return 0;
}


/*****
 * DirectoryMetricsFinder
 **/

DirectoryMetricsFinder::DirectoryMetricsFinder(PermString d)
  : _directory(d)
{
}

Metrics *
DirectoryMetricsFinder::find_metrics_x(PermString name, MetricsFinder *finder,
				       ErrorHandler *errh)
{
  Filename newfile =
    Filename(_directory, permprintf("%p.afm", name.capsule()));
  if (newfile.readable()) {
    Slurper slurper(newfile);
    AfmReader reader(slurper, errh);
    Metrics *afm = reader.take();
    if (afm) finder->record(afm);
    return afm;
  }
  return 0;
}

AmfmMetrics *
DirectoryMetricsFinder::find_amfm_x(PermString name, MetricsFinder *finder,
				    ErrorHandler *errh)
{
  Filename newfile =
    Filename(_directory, permprintf("%p.amfm", name.capsule()));
  if (newfile.readable()) {
    Slurper slurper(newfile);
    AmfmReader reader(slurper, finder, errh);
    AmfmMetrics *amfm = reader.take();
    if (amfm) finder->record(amfm);
    return amfm;
  }
  return 0;
}
