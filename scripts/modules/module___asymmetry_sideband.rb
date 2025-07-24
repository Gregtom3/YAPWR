require_relative 'module_runner'
require 'fileutils'

class AsymmetrySidebandRunner < ModuleRunner
  SIGNAL_REGION = 'M2>0.106&&M2<0.166'.freeze
  BACKGROUND_REGIONS = [
    'M2>0.2&&M2<0.45',
    'M2>0.2&&M2<0.4',
    'M2>0.2&&M2<0.35',
    'M2>0.2&&M2<0.3',
    '(M2>0&&M2<0.08)||(M2>0.2&&M2<0.4)'
  ].freeze

  def module_key
    'asymmetry_sideband'
  end

  # Only real data pi0 pairs
  def keep_leaf?(tag, info_path)
    return false if tag.start_with?('MC_')
    pair = File.basename(File.dirname(File.dirname(info_path))) # config_dir/pair/leaf/tree_info.yaml
    pair.include?('pi0')
  end

  # We override the whole per-leaf flow because we run N commands (one per bkg window)
  def process_leaf(ctx)
    pair        = File.basename(File.dirname(ctx[:leaf_dir]))
    filtered    = File.join(ctx[:leaf_dir], File.basename(ctx[:orig_tfile]))
    return unless File.exist?(filtered)

    cmds = BACKGROUND_REGIONS.map do |bkg|
      sanitized = sanitize(bkg)
      outdir    = File.join(ctx[:leaf_dir], "module-out___asymmetry_sideband_#{sanitized}")
      FileUtils.mkdir_p(outdir)
      root_line(filtered, ctx[:tree_name], pair, outdir, SIGNAL_REGION, bkg)
    end

    run_multi(ctx[:tag], File.dirname(ctx[:info_path]), cmds)
  end

  def slurm_job_name(tag)
    "asym_sb_#{tag}"
  end

  def slurm_directives
    { time: '24:00:00', mem_per_cpu: '4000', cpus: 1 }
  end

  private

  def root_line(filtered, ttree, pair, outdir, sig, bkg)
    %Q{root -l -b -q 'src/modules/asymmetry.C("#{filtered}","#{ttree}","#{pair}","#{outdir}","#{sig}","#{bkg}")'}
  end

  def sanitize(expr)
    expr.gsub(/\s+/, '')
        .gsub(/[^0-9A-Za-z.]/, '_')
        .gsub(/_+/, '_')
        .gsub(/^_|_$/, '')
  end

  # Same helper pattern we used in FilterTreeRunner
  def run_multi(tag, sbatch_dir, cmds)
    if options[:slurm]
      script = write_sbatch_script(tag, sbatch_dir, cmds.join("\n\n"))
      out = `sbatch #{script}`
      if out =~ /Submitted batch job (\d+)/
        @job_ids << $1
        puts "[SLURM_JOBS] #{@job_ids.join(',')}"
      else
        warn "[#{module_key}] sbatch failed for #{tag}: #{out.strip}"
      end
    else
      cmds.each do |cmd|
        puts "[#{module_key}][#{tag}] RUN: #{cmd}"
        system(cmd) or warn "[#{module_key}] ERROR on #{tag}"
      end
    end
  end
end

AsymmetrySidebandRunner.run!
