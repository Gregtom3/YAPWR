require_relative 'module_runner'
require 'fileutils'
require 'yaml'
require 'optparse'

class FilterTreeRunner < ModuleRunner
  # ----------------------------
  # Hooks / overrides
  # ----------------------------
  def module_key
    'filterTree'
  end

  # We need both data & MC leaves; nothing to skip.
  def keep_leaf?(_tag, _info_path) = true

  def needs_filtered_file?
    false
  end
  # We don’t have a single YAML result file; macro writes into leaf dir.
  # So we override the whole per-leaf flow.
  def process_leaf(ctx)
    pair     = File.basename(File.dirname(ctx[:leaf_dir]))
    tag      = ctx[:tag]
    outdir   = ctx[:leaf_dir]
    max_ent  = options[:max_entries]

    # Build the (one or two) ROOT commands
    cmds = []
    cmds << root_line('filterTree.C',   ctx[:orig_tfile], ctx[:tree_name], ctx[:primary_yaml], pair, outdir, max_ent)
    cmds << root_line('filterTreeMC.C', ctx[:orig_tfile], ctx[:tree_name], ctx[:primary_yaml], pair, outdir, max_ent) if tag.start_with?('MC')

    # For Slurm: one script that runs both lines. For local: run sequentially.
    run_multi(tag, outdir, cmds)
  end

  def slurm_job_name(tag)
    "ft_#{tag}"
  end

  # a bit less RAM than the others
  def slurm_directives
    { time: '08:00:00', mem_per_cpu: '2000', cpus: 1 }
  end

  # ----------------------------
  # Helpers
  # ----------------------------
  private

  def root_line(macro, tfile, ttree, yaml_cfg, pair, outdir, max_entries)
    %Q{root -l -q 'src/modules/#{macro}("#{tfile}","#{ttree}","#{yaml_cfg}","#{pair}","#{outdir}",#{max_entries})'}
  end

  # Reuse base run_job machinery but allow multiple commands
  def run_multi(tag, outdir, cmds)
    if options[:slurm]
      script = write_sbatch_script(tag, outdir, cmds.join("\n"))
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

FilterTreeRunner.run!
