require_relative 'module_runner'

class ParticleMisIDRunner < ModuleRunner
  def module_key
    'particleMisidentification'
  end

  def keep_leaf?(tag, _info_path)
    tag.include?('MC')
  end

  def result_yaml_name(_tag)
    'particleMisidentification.yaml'
  end

  def macro_call(ctx)
    %Q{'src/modules/particleMisidentification.C("#{ctx[:orig_tfile]}","#{ctx[:tree_name]}","#{ctx[:primary_yaml]}","#{ctx[:out_yaml]}")'}
  end

  def slurm_job_name(tag)
    "pmisid_#{tag}"
  end
end

ParticleMisIDRunner.run!
