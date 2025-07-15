#pragma once
#include <string>
#include "Config.h"
#include "Result.h"

/// Base class for all analysis‐module processors
/// Each processor reads its own module‐output directory and fills a Result.
class ModuleProcessor {
public:
  virtual ~ModuleProcessor() = default;

  /// Return the name of the module (e.g. "asymmetryPW", "binMigration")
  virtual std::string name() const = 0;

  /// Process one configuration:
  ///   - moduleOutDir: e.g. "./out/proj/config_X/piminus_pi0/Fall2018_RGA_inbending/module-out___asymmetryPW/"
  ///   - cfg: full parsed config (e.g. kinematic bin edges)
  ///   - returns a Result filled with stats, tables, etc.
  virtual Result process(const std::string& moduleOutDir,
                         const Config& cfg) = 0;
};