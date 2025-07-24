#!/usr/bin/env ruby
# coding: utf-8
# module___binMigration.rb — invoke binMigration.C on each MC‑tagged leaf,
#                        passing it the filtered TFile, TTree name,
#                        the top‑level config YAML, and project root.

require 'yaml'
require 'fileutils'
require 'optparse'

# ------------------------------------------------------------------
#  CLI parsing
# ------------------------------------------------------------------
options = { slurm: false, deps: nil }
OptionParser.new do |opts|
  opts.banner = "Usage: #{$0} [--slurm] [--dependency afterok:IDs] PROJECT_NAME [CONFIG...]"
  opts.on('--slurm',    'Submit each job via sbatch instead of running locally') do
    options[:slurm] = true
  end
  opts.on('--dependency D', 'Pass "afterok:IDs" to sbatch --dependency') do |d|
    options[:deps] = d
  end
  opts.on('-h', '--help', 'Show this help message') do
    puts opts
    exit
  end
end.order!

project = ARGV.shift or begin
  STDERR.puts "Usage: #{$0} [--slurm] [--dependency afterok:IDs] PROJECT_NAME [CONFIG...]"
  exit 1
end

out_root = File.join('out', project)
abort "ERROR: '#{out_root}' does not exist" unless Dir.exist?(out_root)
user_configs = ARGV.map { |c| "config_#{File.basename(c, File.extname(c))}" }
job_ids = []

# iterate over all tree_info.yaml under MC-tagged leaves
Dir
  .glob(File.join(out_root, 'config_*', '**', 'tree_info.yaml'))
  .sort
  .each do |info_path|

  tag = File.basename(File.dirname(info_path))
  next unless tag.include?('MC')

  info       = YAML.load_file(info_path)
  orig_tfile = info.fetch('tfile')
  tree_name  = info.fetch('ttree')
  leaf_dir   = File.dirname(info_path)
  filtered   = File.join(leaf_dir, "gen_#{File.basename(orig_tfile)}")

  unless File.exist?(filtered)
    STDERR.puts "[binMigration][#{tag}] WARNING: filtered file not found: #{filtered}"
    next
  end

  # ascend to config_<NAME> directory
  cfg_dir  = leaf_dir
  cfg_dir  = File.dirname(cfg_dir) until File.basename(cfg_dir).start_with?('config_')
  cfg_name = File.basename(cfg_dir).sub(/^config_/, '')
  next if user_configs.any? && user_configs.none? { |c| File.basename(cfg_dir).include?(c) }
  primary_yaml = File.join(cfg_dir, "#{cfg_name}.yaml")

  unless File.exist?(primary_yaml)
    STDERR.puts "[binMigration][#{tag}] WARNING: primary YAML not found: #{primary_yaml}"
    next
  end

  # prepare output directory and log file
  outdir   = File.join(leaf_dir, 'module-out___binMigration')
  FileUtils.mkdir_p(outdir)
  log_file = File.join(outdir, 'binMigration.yaml')

  puts "[binMigration][#{tag}]"
  puts "    filtered:     #{filtered}"
  puts "    primary yaml: #{primary_yaml}"
  puts "    log file:     #{log_file}"

  # build the ROOT macro invocation
  macro = %Q{'src/modules/binMigration.C(
    "#{orig_tfile}",
    "#{tree_name}",
    "#{primary_yaml}",
    "#{out_root}",
    "#{log_file}" )'}
  cmd = ['root', '-l', '-b', '-q', macro].join(' ')

  if options[:slurm]
    # write a lightweight sbatch script
    sbatch = <<~SLURM
      #!/bin/bash
      #SBATCH --job-name=binMig_#{tag}
      #SBATCH --output=#{outdir}/binMigration_#{tag}.out
      #SBATCH --error=#{outdir}/binMigration_#{tag}.err
      #SBATCH --time=24:00:00
      #SBATCH --mem-per-cpu=1000
      #SBATCH --cpus-per-task=1
      #SBATCH --dependency=#{options[:deps]}

      cd #{Dir.pwd}
      #{cmd}
    SLURM

    script_file = File.join(outdir, "run_binMigration_#{tag}.slurm")
    File.write(script_file, sbatch)
    FileUtils.chmod('+x', script_file)

    # submit and capture job ID
    out = `sbatch #{script_file}`
    if out =~ /Submitted batch job (\d+)/
      id = $1
      job_ids << id
      puts "[SLURM_JOBS] #{job_ids.join(',')}"
    else
      warn "[binMigration] sbatch failed for #{tag}: #{out.strip}"
    end
  else
    # direct execution
    puts "[binMigration][#{tag}] RUN: #{cmd}"
    system(cmd) or warn "[binMigration] ERROR on #{primary_yaml}" 
  end
end

# if in slurm mode, dump all submitted job IDs for upstream
if options[:slurm] && job_ids.any?
  puts "[SLURM_JOBS] #{job_ids.join(',')}"
end
