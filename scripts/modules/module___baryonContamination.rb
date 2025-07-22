#!/usr/bin/env ruby
# coding: utf-8
#
# module___baryonContamination.rb – run baryonContamination.C
# on every MC‑tagged leaf, with optional Slurm submission

require 'yaml'
require 'fileutils'
require 'optparse'

# ------------------------------------------------------------------
# CLI parsing
# ------------------------------------------------------------------
options = { slurm: false, deps: nil }
OptionParser.new do |opts|
  opts.banner = "Usage: #{$0} [--slurm] [--dependency afterok:IDs] PROJECT_NAME"
  opts.on('--slurm', 'Submit each job via sbatch instead of running locally') do
    options[:slurm] = true
  end
  opts.on('--dependency D', 'SBATCH --dependency string (e.g. afterok:1234)') do |d|
    options[:deps] = d
  end
  opts.on('-h', '--help', 'Show this help message') do
    puts opts
    exit
  end
end.order!

project = ARGV.shift or abort("Usage: #{$0} [--slurm] [--dependency afterok:IDs] PROJECT_NAME")
out_root = File.join("out", project)
abort "ERROR: '#{out_root}' does not exist" unless Dir.exist?(out_root)

job_ids = []

Dir.glob(File.join(out_root, "config_*", "**", "tree_info.yaml")).sort.each do |info_path|
  tag = File.basename(File.dirname(info_path))
  next unless tag.include?("MC")

  info         = YAML.load_file(info_path)
  orig_tfile   = info.fetch("tfile")
  tree_name    = info.fetch("ttree")
  leaf_dir     = File.dirname(info_path)
  filtered_tfile = File.join(leaf_dir, File.basename(orig_tfile))

  unless File.exist?(filtered_tfile)
    STDERR.puts "[baryonContamination][#{tag}] WARNING: filtered file not found: #{filtered_tfile}"
    next
  end

  outdir   = File.join(leaf_dir, "module-out___baryonContamination")
  FileUtils.mkdir_p(outdir)
  log_file = File.join(outdir, "baryonContamination.yaml")

  # ascend to config_<NAME>
  cfg_dir     = leaf_dir
  cfg_dir     = File.dirname(cfg_dir) until File.basename(cfg_dir).start_with?("config_")
  cfg_name    = File.basename(cfg_dir).sub(/^config_/, '')
  primary_yaml = File.join(cfg_dir, "#{cfg_name}.yaml")
  unless File.exist?(primary_yaml)
    STDERR.puts "[baryonContamination][#{tag}] WARNING: primary YAML not found: #{primary_yaml}"
    next
  end

  macro = %Q{'src/modules/baryonContamination.C(
    "#{orig_tfile}",
    "#{tree_name}",
    "#{primary_yaml}",
    "#{log_file}"
  )'}
  cmd = ['root', '-l', '-b', '-q', macro]

  if options[:slurm]
    sbatch = <<~SBATCH
      #!/bin/bash
      #SBATCH --job-name=baryonCont_#{tag}
      #SBATCH --output=#{outdir}/baryonContamination_#{tag}.out
      #SBATCH --error=#{outdir}/baryonContamination_#{tag}.err
      #SBATCH --time=24:00:00
      #SBATCH --mem-per-cpu=4000
      #SBATCH --cpus-per-task=1
    SBATCH
    sbatch << "#SBATCH --dependency=#{options[:deps]}\n" if options[:deps]
    sbatch << <<~SBATCH
      cd #{Dir.pwd}
      #{cmd.join(' ')}
    SBATCH

    script = File.join(outdir, "run_baryonContamination_#{tag}.slurm")
    File.write(script, sbatch)
    FileUtils.chmod('+x', script)

    out = `sbatch #{script}`
    if out =~ /Submitted batch job (\d+)/
      job_ids << $1
      puts "[SLURM_JOBS] #{job_ids.join(',')}"
    else
      warn "[baryonContamination] sbatch failed for #{tag}: #{out.strip}"
    end
  else
    puts "[baryonContamination][#{tag}] RUN: #{cmd.join(' ')}"
    system(*cmd) or
      STDERR.puts("[baryonContamination] ERROR: ROOT macro failed for #{info_path}")
  end
end

if options[:slurm] && job_ids.any?
  puts "[SLURM_JOBS] #{job_ids.join(',')}"
end
