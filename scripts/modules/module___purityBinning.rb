require_relative 'module_runner'
require 'fileutils'

class PurityBinningRunner < ModuleRunner
  def module_key
    'purityBinning'
  end

  # We only care about π⁰ pairs; don't exclude MC unless you want to.
  # (Your original script didn't.)
  def keep_leaf?(_tag, info_path)
    pair = File.basename(File.dirname(File.dirname(info_path)))
    pair.include?('pi0')
  end

  # One command per leaf, so we just override process_leaf.
  def process_leaf(ctx)
    pair     = File.basename(File.dirname(ctx[:leaf_dir]))
    filtered = File.join(ctx[:leaf_dir], File.basename(ctx[:orig_tfile]))
    unless File.exist?(filtered)
      warn "[#{module_key}][#{pair}] filtered file not found: #{filtered}"
      return
    end

    outdir = File.join(ctx[:leaf_dir], out_subdir)
    FileUtils.mkdir_p(outdir)

    # Build cmd and run
    cmd = build_root_cmd(ctx.merge(pair: pair, outdir: outdir, filtered_tfile: filtered))
    run_job(ctx[:tag], outdir, cmd)
  end

  # ROOT macro signature:
  # purityBinning.C(filtered_file, ttree, pair, output_dir)
  def macro_call(ctx)
    %Q{'src/modules/purityBinning.C("#{ctx[:filtered_tfile]}","#{ctx[:tree_name]}","#{ctx[:pair]}","#{ctx[:outdir]}")'}
  end

  def slurm_job_name(tag)
    "purityBin_#{tag}"
  end

  def slurm_directives
    { time: '24:00:00', mem_per_cpu: '2000', cpus: 1 }
  end
end

PurityBinningRunner.run!
