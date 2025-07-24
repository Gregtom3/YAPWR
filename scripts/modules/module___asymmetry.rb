require_relative 'module_runner'
require 'fileutils'

class AsymmetryPWRunner < ModuleRunner
  def module_key
    'asymmetryPW'
  end

  # Data only: skip MC_ tags
  def keep_leaf?(tag, _info_path)
    !tag.start_with?('MC_')
  end

  def process_leaf(ctx)
    pair   = File.basename(File.dirname(ctx[:leaf_dir]))
    outdir = File.join(ctx[:leaf_dir], out_subdir)
    FileUtils.mkdir_p(outdir)

    cmd = build_root_cmd(ctx.merge(pair: pair, outdir: outdir))
    run_job(ctx[:tag], outdir, cmd)
  end

  def macro_call(ctx)
    %Q{'src/modules/asymmetry.C("#{ctx[:filtered_tfile]}","#{ctx[:tree_name]}","#{ctx[:pair]}","#{ctx[:outdir]}")'}
  end

  def slurm_job_name(tag)
    "asym_#{tag}"
  end

  def slurm_directives
    { time: '24:00:00', mem_per_cpu: '4000', cpus: 1 }
  end
end

AsymmetryPWRunner.run!
