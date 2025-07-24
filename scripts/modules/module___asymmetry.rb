#!/usr/bin/env ruby
require 'yaml'
require 'fileutils'
require 'optparse'

options = { slurm: false, deps: nil }

OptionParser.new do |opts|
  opts.banner = "Usage: #{$0} [--slurm] [--dependency afterok:IDs] PROJECT_NAME [CONFIG...]"

  opts.on('--slurm', 'Submit via sbatch') do
    options[:slurm] = true
  end

  # Fix: accept the dependency string into a block variable `d`
  opts.on('--dependency D', 'SBATCH --dependency string (e.g. afterok:1234)') do |d|
    options[:deps] = d
  end

  opts.on('-h', '--help', 'Show this help') do
    puts opts
    exit
  end
end.order!

project_name = ARGV.shift or abort "Usage: #{$0} [--slurm] [--dependency afterok:IDs] PROJECT_NAME [CONFIG...]"
user_configs = ARGV.map { |c| "config_#{File.basename(c, File.extname(c))}" }

out_root = File.join("out", project_name)
abort "no out dir" unless Dir.exist?(out_root)

job_ids = []

Dir.glob(File.join(out_root, "config_*")).sort.each do |config_dir|
  Dir.glob(File.join(config_dir, "**", "tree_info.yaml")).sort.each do |info_path|
    cfg_name = File.basename(config_dir)
    next if user_configs.any? && user_configs.none? { |c| cfg_name.include?(c) }
    info       = YAML.load_file(info_path)
    orig_tfile = info.fetch("tfile")
    ttree      = info.fetch("ttree")

    leaf = File.dirname(info_path)
    pair = File.basename(File.dirname(leaf))
    filtered = File.join(leaf, File.basename(orig_tfile))
    tag  = File.basename(leaf)
    # **skip Monte-Carlo tags**
    next if tag.start_with?("MC_")
      
    unless File.exist?(filtered)
      warn "[module___asymmetryPW] missing filtered: #{filtered}"
      next
    end

    mod_out = File.join(leaf, "module-out___asymmetryPW")
    FileUtils.mkdir_p(mod_out)

    root_cmd = [
      "root", "-l", "-b", "-q",
      %Q{'src/modules/asymmetry.C("#{filtered}","#{ttree}","#{pair}","#{mod_out}")'}
    ].join(' ')

    if options[:slurm]
      # write a tiny sbatch script
      script = <<~SL
        #!/bin/bash
        #SBATCH --job-name=asym_#{pair}
        #SBATCH --output=#{mod_out}/asym_#{pair}.out
        #SBATCH --error=#{mod_out}/asym_#{pair}.err
        #SBATCH --time=24:00:00
        #SBATCH --mem-per-cpu=4000
        #SBATCH --cpus-per-task=1
        # optionally depend on upstream
        #SBATCH --dependency=#{options[:deps]}        
        cd #{Dir.pwd}
        #{root_cmd}
      SL

      sbatch_file = File.join(mod_out, "run_asym_#{pair}.slurm")
      File.write(sbatch_file, script)
      FileUtils.chmod('+x', sbatch_file)

      # submit & capture its output
      out = `sbatch #{sbatch_file}`
      # out = "Submitted batch job 123456\n"
      if out =~ /Submitted batch job (\d+)/
        id = $1
        job_ids << id
        puts "[SLURM_JOBS] #{job_ids.join(',')}"
      else
        warn "[module___asymmetryPW] sbatch failed: #{out}"
      end
    else
      # direct run
      puts "[module___asymmetry] Running: #{root_cmd}"
      system(root_cmd) or warn "[module___asymmetryPW] ERROR for #{info_path}"
    end
  end
end

# if in slurm mode, print one last line so run_project can pick it up
if options[:slurm] && job_ids.any?
  puts "[SLURM_JOBS] #{job_ids.join(',')}"
end
