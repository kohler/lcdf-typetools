#ifndef PSRES_HH
#define PSRES_HH
#ifdef __GNUG__
#pragma interface
#endif
#include "vector.hh"
#include "hashmap.hh"
#include "string.hh"
#include "filename.hh"
class PsresDatabaseSection;
class Slurper;

class PsresDatabase {
  
  HashMap<PermString, int> _section_map;
  Vector<PsresDatabaseSection *> _sections;
  
  PsresDatabaseSection *force_section(PermString);
  bool add_one_psres_file(Slurper &, bool override);
  void add_psres_directory(PermString);
  
 public:
  
  PsresDatabase();
  ~PsresDatabase();
  
  void add_psres_path(const char *path, const char *defaults, bool override);
  bool add_psres_file(Filename &, bool override);
  void add_database(PsresDatabase *, bool override);
  
  PsresDatabaseSection *section(PermString section) const;
  const String &value(PermString section, PermString key) const;
  const String &unescaped_value(PermString section, PermString key) const;
  Filename filename_value(PermString section, PermString key) const;
  
};

class PsresDatabaseSection {
  
  PermString _section_name;
  HashMap<PermString, int> _map;
  Vector<PermString> _directories;
  Vector<String> _values;
  Vector<bool> _value_escaped;
  
  const String &value(int index);
  
 public:
  
  PsresDatabaseSection(PermString);
  
  PermString section_name() const		{ return _section_name; }
  
  void add_psres_file_section(Slurper &, PermString, bool);
  void add_section(PsresDatabaseSection *, bool override);
  
  const String &value(PermString key)		{ return value(_map[key]); }
  const String &unescaped_value(PermString key) const;
  Filename filename_value(PermString key);
  
};

inline PsresDatabaseSection *
PsresDatabase::section(PermString n) const
{
  return _sections[_section_map[n]];
}

inline const String &
PsresDatabaseSection::unescaped_value(PermString key) const
{
  assert(!_value_escaped[_map[key]]);
  return _values[_map[key]];
}

#endif
