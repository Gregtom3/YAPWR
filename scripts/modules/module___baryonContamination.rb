require_relative 'module_runner'

class BaryonContaminationRunner < ModuleRunner
  def module_key
    'baryonContamination'
  end

  def keep_leaf?(tag, _info_path)
    tag.include?('MC')
  end

  def result_yaml_name(_tag)
    'baryonContamination.yaml'
  end

  def macro_call(ctx)
    %Q{'src/modules/baryonContamination.C("#{ctx[:orig_tfile]}","#{ctx[:tree_name]}","#{ctx[:primary_yaml]}","#{ctx[:out_yaml]}")'}
  end

  def slurm_job_name(tag)
    "baryonCont_#{tag}"
  end
end

BaryonContaminationRunner.run!