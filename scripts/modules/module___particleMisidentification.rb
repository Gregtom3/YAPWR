#!/usr/bin/env ruby
# coding: utf-8
#
# module___particleMisidentification.rb – run particleMisidentification.C
# on every filtered tree in out/<PROJECT>, with optional Slurm

require 'yaml'
require 'fileutils'
require 'optparse'

# ------------------------------------------------------------------
#  CLI parsing
# ------------------------------------------------------------------
options = { slurm: false, deps: nil }
OptionParser.new do |opts|
  opts.banner = "Usage: #{$0} [--slurm] [--dependency afterok:IDs] PROJECT_NAME [CONFIG...]"
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

project = ARGV.shift or abort("Usage: #{$0} [--slurm] [--dependency afterok:IDs] PROJECT_NAME [CONFIG...]")
user_configs = ARGV.map { |c| "config_#{File.basename(c, File.extname(c))}" }
out_root = File.join("out", project)
abort "ERROR: '#{out_root}' does not exist" unless Dir.exist?(out_root)

job_ids = []

Dir.glob(File.join(out_root, "config_*", "**", "tree_info.yaml")).sort.each do |info_path|
  tag = File.basename(File.dirname(info_path))
  next unless tag.include?("MC")      # only MC‑tagged subtrees

  info           = YAML.load_file(info_path)
  orig_tfile     = info.fetch("tfile")
  tree_name      = info.fetch("ttree")
  leaf_dir       = File.dirname(info_path)
  filtered_tfile = File.join(leaf_dir, File.basename(orig_tfile))

  unless File.exist?(filtered_tfile)
    STDERR.puts "[particleMisidentification][#{tag}] WARNING: filtered file not found: #{filtered_tfile}"
    next
  end

  # prepare output directory + yaml path
  outdir   = File.join(leaf_dir, "module-out___particleMisidentification")
  FileUtils.mkdir_p(outdir)
  yaml_path = File.join(outdir, "particleMisidentification.yaml")



  # ascend to config_<NAME> dir
  cfg_dir      = leaf_dir
  cfg_dir      = File.dirname(cfg_dir) until File.basename(cfg_dir).start_with?("config_")
  cfg_name     = File.basename(cfg_dir).sub(/^config_/, '')
  next if user_configs.any? && user_configs.none? { |c| File.basename(cfg_dir).include?(c) }
  puts "[particleMisidentification][#{tag}] #{filtered_tfile} → #{yaml_path}"
  primary_yaml = File.join(cfg_dir, "#{cfg_name}.yaml")
  unless File.exist?(primary_yaml)
    STDERR.puts "[particleMisidentification][#{tag}] WARNING: primary YAML not found: #{primary_yaml}"
    next
  end

  # build ROOT macro command
  macro = %Q{'src/modules/particleMisidentification.C("#{orig_tfile}","#{tree_name}","#{primary_yaml}","#{yaml_path}")'}
  cmd = ['root', '-l', '-b', '-q', macro].join(' ')

  if options[:slurm]
    # write sbatch script
    sbatch = <<~SLURM
      #!/bin/bash
      #SBATCH --job-name=pmisid_#{tag}
      #SBATCH --output=#{outdir}/pmisid_#{tag}.out
      #SBATCH --error=#{outdir}/pmisid_#{tag}.err
      #SBATCH --time=24:00:00
      #SBATCH --mem-per-cpu=1000
      #SBATCH --cpus-per-task=1
    SLURM
    sbatch += "#SBATCH --dependency=#{options[:deps]}\n" if options[:deps]
    sbatch += <<~SLURM
      cd #{Dir.pwd}
      #{cmd}
    SLURM

    script_file = File.join(outdir, "run_pmisid_#{tag}.slurm")
    File.write(script_file, sbatch)
    FileUtils.chmod('+x', script_file)

    out = `sbatch #{script_file}`
    if out =~ /Submitted batch job (\d+)/
      job_ids << $1
      puts "[SLURM_JOBS] #{job_ids.join(',')}"
    else
      warn "[particleMisidentification] sbatch failed for #{tag}: #{out.strip}"
    end

  else
    # direct execution
    puts "[particleMisidentification][#{tag}] RUN: #{cmd}"
    system(cmd) or warn "[particleMisidentification] ERROR running for #{tag}"
  end
end

# final dump of job IDs
if options[:slurm] && job_ids.any?
  puts "[SLURM_JOBS] #{job_ids.join(',')}"
end
