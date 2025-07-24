#!/usr/bin/env ruby
# coding: utf-8
#
# module___purityBinning.rb – run purityBinning.C on every pi0‐tagged leaf,
#                            with optional Slurm submission

require 'yaml'
require 'fileutils'
require 'optparse'

# ------------------------------------------------------------------
# CLI parsing
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
  opts.on('-h', '--help', 'Show help') do
    puts opts
    exit
  end
end.order!

project_name = ARGV.shift or abort("Usage: #{$0} [--slurm] [--dependency afterok:IDs] PROJECT_NAME [CONFIG...]")
user_configs = ARGV.map { |c| "config_#{File.basename(c, File.extname(c))}" }

out_root = File.join("out", project_name)
abort "ERROR: '#{out_root}' does not exist" unless Dir.exist?(out_root)
puts "[INFO] Filtering configs: #{user_configs.join(', ')}" unless user_configs.empty?

job_ids = []

Dir.glob(File.join(out_root, "config_*")).sort.each do |config_dir|
  Dir.glob(File.join(config_dir, "**", "tree_info.yaml")).sort.each do |info_path|
    cfg_name = File.basename(config_dir)
    next if user_configs.any? && user_configs.none? { |c| cfg_name.include?(c) }
    info       = YAML.load_file(info_path)
    orig_tfile = info.fetch("tfile")
    ttree      = info.fetch("ttree")
    leaf_dir   = File.dirname(info_path)
    pair       = File.basename(File.dirname(leaf_dir))
    next unless pair.include?("pi0")

    filtered_file = File.join(leaf_dir, File.basename(orig_tfile))
    unless File.exist?(filtered_file)
      STDERR.puts "[purityBinning][#{pair}] WARNING: filtered file not found: #{filtered_file}"
      next
    end

    output_dir = File.join(leaf_dir, "module-out___purityBinning")
    FileUtils.mkdir_p(output_dir)

    primary_yaml = begin
      cfg_dir = leaf_dir
      cfg_dir = File.dirname(cfg_dir) until File.basename(cfg_dir).start_with?("config_")
      name = File.basename(cfg_dir).sub(/^config_/, '')
      File.join(cfg_dir, "#{name}.yaml")
    end
    unless File.exist?(primary_yaml)
      STDERR.puts "[purityBinning][#{pair}] WARNING: primary YAML not found: #{primary_yaml}"
      next
    end

    # build ROOT macro command
    macro = %Q{'src/modules/purityBinning.C("#{filtered_file}","#{ttree}","#{pair}","#{output_dir}")'}
        
    root_cmd = ['root', '-l', '-b', '-q', macro].join(' ')
    

    if options[:slurm]
      sbatch = <<~SBATCH
        #!/bin/bash
        #SBATCH --job-name=purityBin_#{pair}
        #SBATCH --output=#{output_dir}/purityBinning_#{pair}.out
        #SBATCH --error=#{output_dir}/purityBinning_#{pair}.err
        #SBATCH --time=24:00:00
        #SBATCH --mem-per-cpu=2000
        #SBATCH --cpus-per-task=1
      SBATCH
      sbatch << "#SBATCH --dependency=#{options[:deps]}\n" if options[:deps]
      sbatch << <<~SBATCH
        cd #{Dir.pwd}
        #{root_cmd}
      SBATCH

      script = File.join(output_dir, "run_purityBinning_#{pair}.slurm")
      File.write(script, sbatch)
      FileUtils.chmod('+x', script)

      out = `sbatch #{script}`
      if out =~ /Submitted batch job (\d+)/
        job_ids << $1
        puts "[SLURM_JOBS] #{job_ids.join(',')}"
      else
        warn "[purityBinning] sbatch failed for #{pair}: #{out.strip}"
      end
    else
      puts "[purityBinning][#{pair}] RUN: #{root_cmd}"
      system(root_cmd) or
        STDERR.puts("[purityBinning] ERROR: purityBinning failed for #{info_path}")
    end
  end
end

if options[:slurm] && job_ids.any?
  puts "[SLURM_JOBS] #{job_ids.join(',')}"
end
