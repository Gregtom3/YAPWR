require_relative 'module_runner'
require 'fileutils'

class BinMigrationRunner < ModuleRunner
  def module_key
    'binMigration'
  end

  # Only MC-tagged leaves
  def keep_leaf?(tag, _info_path)
    tag.include?('MC')
  end

  def process_leaf(ctx)
    # filtered file your script looked for:
    filtered = File.join(ctx[:leaf_dir], "gen_#{File.basename(ctx[:orig_tfile])}")
    unless File.exist?(filtered)
      warn "[#{module_key}][#{ctx[:tag]}] filtered file not found: #{filtered}"
      return
    end

    outdir   = File.join(ctx[:leaf_dir], out_subdir)
    FileUtils.mkdir_p(outdir)
    log_file = File.join(outdir, 'binMigration.yaml')

    # Build and run
    cmd = build_root_cmd(ctx.merge(filtered_tfile: filtered, outdir: outdir, log_file: log_file))
    run_job(ctx[:tag], outdir, cmd)
  end

  def macro_call(ctx)
    %Q{'src/modules/binMigration.C("#{ctx[:orig_tfile]}","#{ctx[:tree_name]}","#{ctx[:primary_yaml]}","#{out_root}","#{ctx[:log_file]}")'}
  end

  def slurm_job_name(tag)
    "binMig_#{tag}"
  end

  def slurm_directives
    { time: '24:00:00', mem_per_cpu: '1000', cpus: 1 }
  end
end

BinMigrationRunner.run!
