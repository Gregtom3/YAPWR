require_relative 'module_runner'
require 'fileutils'

class KinematicBinsRunner < ModuleRunner
  def module_key
    'kinematicBins'
  end

  # run on anything that actually has a filtered file
  def keep_leaf?(_tag, info_path)
    info   = YAML.load_file(info_path)
    leaf   = File.dirname(info_path)
    filtered = File.join(leaf, File.basename(info['tfile']))
    File.exist?(filtered)
  end

  def process_leaf(ctx)
    pair      = File.basename(File.dirname(ctx[:leaf_dir]))
    outdir = File.join(ctx[:leaf_dir], out_subdir)
    FileUtils.mkdir_p(outdir)

    cmd = build_root_cmd(ctx.merge(pair: pair,
                                   outdir: outdir))
    run_job(ctx[:tag], outdir, cmd)
  end

  def macro_call(ctx)
    %Q{'src/modules/kinematicBins.C("#{ctx[:orig_tfile]}","#{ctx[:tree_name]}","#{ctx[:pair]}","#{ctx[:primary_yaml]}","#{ctx[:outdir]}")'}
  end

  def slurm_job_name(tag)
    "kinBins_#{tag}"
  end

  def slurm_directives
    { time: '24:00:00', mem_per_cpu: '1000', cpus: 1 }
  end
end

KinematicBinsRunner.run!
