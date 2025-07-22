#!/usr/bin/env ruby
# ------------------------------------------------------------------
#  module___filterTree.rb   (now Slurm‑aware)
#  Usage:
#     ruby module___filterTree.rb [--slurm] [--dependency afterok:IDs] \
#                                 PROJECT_NAME [maxEntries]
# ------------------------------------------------------------------
require 'optparse'
require 'yaml'
require 'fileutils'

# ---------- CLI ---------------------------------------------------
opts = { slurm: false, deps: nil }
OptionParser.new do |o|
  o.on('--slurm')               { opts[:slurm] = true }
  o.on('--dependency D') { |d|  opts[:deps]  = d }
end.order!   # leaves the rest in ARGV

project_name = ARGV.shift or abort "Usage: #{$0} [--slurm] PROJECT_NAME [maxEntries]"
max_entries  = ARGV.shift&.to_i
entry_arg    = (max_entries && max_entries.positive?) ? max_entries : -1

out_root = File.join('out', project_name)
abort "ERROR: output root '#{out_root}' does not exist" unless Dir.exist?(out_root)

job_ids = []                        # collect sbatch IDs (slurm mode only)

# ---------- iterate over all configs / trees ----------------------
Dir.glob(File.join(out_root, 'config_*')).sort.each do |config_dir|
  cfg = Dir.glob(File.join(config_dir, '*.yaml'))
          .reject { |f| f.end_with?('tree_info.yaml') }
          .first or (warn("no config yaml in #{config_dir}") ; next)

  puts "\n== Processing #{File.basename(config_dir)} =="

  Dir.glob(File.join(config_dir, '**', 'tree_info.yaml')).sort.each do |info_path|
    info   = YAML.load_file(info_path)
    tfile  = info.fetch('tfile')
    ttree  = info.fetch('ttree')
    leaf   = File.dirname(info_path)
    tag    = File.basename(leaf)
    pair   = File.basename(File.dirname(leaf))
    outdir = leaf

    root_cmds = []
    root_cmds << "root -l -q src/modules/filterTree.C\\(\\\"#{tfile}\\\",\\\"#{ttree}\\\",\\\"#{cfg}\\\",\\\"#{pair}\\\",\\\"#{outdir}\\\",#{entry_arg}\\)"
    if tag.start_with?('MC')
      root_cmds << "root -l -q src/modules/filterTreeMC.C\\(\\\"#{tfile}\\\",\\\"#{ttree}\\\",\\\"#{cfg}\\\",\\\"#{pair}\\\",\\\"#{outdir}\\\",#{entry_arg}\\)"
    end

    if opts[:slurm]
      # ------------- write & submit sbatch ------------------------
      job_name = "ft_#{pair}_#{tag}"
      sbatch   = <<~SLURM
        #!/bin/bash
        #SBATCH --job-name=#{job_name}
        #SBATCH --output=#{outdir}/filterTree.out
        #SBATCH --error=#{outdir}/filterTree.err
        #SBATCH --time=08:00:00
        #SBATCH --mem-per-cpu=2000
        #SBATCH --cpus-per-task=1
        #{"#SBATCH --dependency=#{opts[:deps]}" if opts[:deps]}
        cd #{Dir.pwd}
        #{root_cmds.join("\n")}
      SLURM

      sbatch_path = File.join(outdir, 'run_filterTree.slurm')
      File.write(sbatch_path, sbatch)
      FileUtils.chmod('+x', sbatch_path)

      out = `sbatch #{sbatch_path}`
      if out =~ /Submitted batch job (\d+)/
        job_ids << Regexp.last_match(1)
        puts "[SLURM_JOBS] #{job_ids.join(',')}"
      else
        warn "[filterTree] sbatch failed for #{sbatch_path}: #{out}"
      end
    else
      # ------------- immediate execution --------------------------
      root_cmds.each do |cmd|
        puts "[filterTree] Running: #{cmd}"
        system(cmd) or warn "[filterTree] ERROR on #{cmd}"
      end
    end
  end
end

# final ID list for run_project.rb
puts "[SLURM_JOBS] #{job_ids.join(',')}" if opts[:slurm] && job_ids.any?

