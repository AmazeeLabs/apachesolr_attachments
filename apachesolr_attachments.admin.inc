<?php

/**
 * @file
 * Provides a file attachment search implementation for use with the Apache Solr module
 */

/**
 * Menu callback: Apache Solr Attachments settings tab.
 */
function apachesolr_attachments_admin_page() {
  $output['apachesolr_attachments_settings'] = drupal_get_form('apachesolr_attachments_settings');
  $output['apachesolr_attachments_index_action_form'] = drupal_get_form('apachesolr_attachments_index_action_form');
  return $output;
}

/**
 * Displays the Attachment Settings Form.
 *
 * @see apachesolr_attachments_settings_validate()
 * @see apachesolr_attachments_settings_submit()
 */
function apachesolr_attachments_settings($form) {
  $default = implode(' ', apachesolr_attachments_default_excluded());
  $form['apachesolr_attachment_excluded_extensions'] = array(
    '#type' => 'textfield',
    '#title' => t('Excluded file extensions'),
    '#default_value' => variable_get('apachesolr_attachment_excluded_extensions', $default),
    '#size' => 80,
    '#maxlength' => 255,
    '#description' => t('File extensions that are excluded from indexing. Separate extensions with a space and do not include the leading dot. Extensions are internally mapped to a MIME type, so it is not necessary to put variations that map to the same type (e.g. tif is sufficient for tif and tiff)'),
  );
  $form['apachesolr_attachments_exclude_types'] = array(
    '#type' => 'radios',
    '#title' => t('Exclude files attached to a node of a type excluded by Apache Solr Search'),
    '#options' => array(
      '0' => t('No'),
      '1' => t('Yes'),
    ),
    '#default_value' => variable_get('apachesolr_attachments_exclude_types', 1),
  );
  $form['apachesolr_attachment_extract_using'] = array(
    '#type' => 'radios',
    '#title' => t('Extract using'),
    '#options' => array(
      'tika' => t('Tika (local java application)'),
      'solr' => t('Solr (remote server)'),
    ),
    '#description' => t("Extraction will be faster if run locally using tika."),
    '#default_value' => variable_get('apachesolr_attachment_extract_using', 'tika'),
  );
  $form['apachesolr_attachments_tika_path'] = array(
    '#type' => 'textfield',
    '#title' => t('Tika directory path'),
    '#size' => 80,
    '#maxlength' => 100,
    '#description' => t("The full path to tika directory.  All library jars must be in the same directory."),
    '#default_value' => variable_get('apachesolr_attachments_tika_path', ''),
  );
  $form['apachesolr_attachments_tika_jar'] = array(
    '#type' => 'textfield',
    '#title' => t('Tika jar file'),
    '#size' => 20,
    '#description' => t("The name of the tika CLI application jar file, e.g. tika-0.3.jar or tika-app-0.4.jar."),
    '#default_value' => variable_get('apachesolr_attachments_tika_jar', 'tika-0.3.jar'),
  );
  $form['apachesolr_attachments-cron-settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('Cron settings'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['apachesolr_attachments-cron-settings']['apachesolr_attachments_cron_limit'] = array(
    '#type' => 'select',
    '#title' => t('Maximum number of nodes to examine'),
    '#default_value' => variable_get('apachesolr_attachments_cron_limit', 100),
    '#options' => drupal_map_assoc(array(10, 20, 50, 100, 200)),
  );
  $form['apachesolr_attachments-cron-settings']['apachesolr_attachements_cron_time_limit'] = array(
    '#type' => 'select',
    '#title' => t('Maximum time to expend (sec)'),
    '#default_value' => variable_get('apachesolr_attachements_cron_time_limit', 15),
    '#options' => drupal_map_assoc(array(5, 10, 15, 20, 25, 30, 45, 60)),
  );

  $form = system_settings_form($form);
  $form['#validate'][] = 'apachesolr_attachments_settings_validate';
  $form['#submit'][] = 'apachesolr_attachments_settings_submit';
  return $form;
}

/**
 * Form validation for the Apache Solr Attachments settings form.
 *
 * @see apachesolr_attachments_settings()
 */
function apachesolr_attachments_settings_validate($form, &$form_state) {
  if ($form_state['values']['apachesolr_attachment_extract_using'] == 'tika') {
    $path = realpath($form_state['values']['apachesolr_attachments_tika_path']);
    if (!file_exists($path . '/' . $form_state['values']['apachesolr_attachments_tika_jar'])) {
      form_set_error('apachesolr_attachments_tika_path', t('Tika jar file not found at this path.'));
    }
  }
}

/**
 * Form submit handler for the settings Form
 *
 * @see apachesolr_attachments_settings()
 */
function apachesolr_attachments_settings_submit($form, &$form_state) {
  // Delete this so it's rebuilt.
  variable_del('apachesolr_attachment_excluded_mime');
  drupal_set_message(t('If you changed the allowed file extensions, you may need to delete and re-index all attachments.'));
}

/**
 * Create a form for deleting the contents of the Solr index.
 */
function apachesolr_attachments_index_action_form() {
  $form = array();

  $form['action'] = array(
    '#type' => 'fieldset',
    '#title' => t('Index Actions'),
  );
  // Jump through some forms hoops to get a description under each radio button.
  $actions = array(
    'reindex' => array(
      'title' => t('Re-index all files'),
      'description' => t('Re-indexing will add all file text to the index again (overwriting the index), but existing text in the index will remain searchable.'),
    ),
    'delete' => array(
      'title' => t('Delete files from index'),
      'description' => t('Deletes all of the files in the Solr index and reindexes them. This may be needed if you have changed the allowed file extensions, if your index is corrupt, or if you have installed a new schema.xml.'),
    ),
    'clear-cache' => array(
      'title' => t('Delete cached file text'),
      'description' => t('Deletes the local cache of extracted text from files. This will cause slower performance when reindexing since text must be re-extracted.'),
    ),
  );
  foreach ($actions as $key => $action) {
    // Generate the parents as the autogenerator does, so we will have a
    // unique id for each radio button.
    $form['action'][$key] = array(
      '#type' => 'radio',
      '#title' => $action['title'],
      '#default_value' => 'remaining',
      '#return_value' => $key,
      '#parents' => array('action'),
      '#description' => $action['description'],
      '#id' => drupal_clean_css_identifier('edit-'. implode('-', array('action', $key))),
    );
  }

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Begin'),
    '#submit' => array('apachesolr_attachments_index_action_form_submit'),
  );

  return $form;
}

/**
 * Submit function for the index action form.
 */
function apachesolr_attachments_index_action_form_submit($form, &$form_state) {
  switch ($form_state['values']['action']) {
    case 'reindex':
      $form_state['redirect'] = 'admin/config/search/apachesolr/attachments/confirm/reindex';
      break;

    case 'delete':
      $form_state['redirect'] = 'admin/config/search/apachesolr/attachments/confirm/delete';
      break;

    case 'clear-cache':
      $form_state['redirect'] = 'admin/config/search/apachesolr/attachments/confirm/clear-cache';
      break;
  }
}

/**
 * Index confirmation form
 *
 * @see apachesolr_attachments_confirm_submit()
 */
function apachesolr_attachments_confirm($form, $form_state, $operation) {
  $form = array();
  $form['operation'] = array(
    '#type' => 'value',
    '#value' => $operation,
  );
  switch ($operation) {
    case 'reindex':
      $text = t('Are you sure you want to re-index the text of all file attachments?');
      break;
    case 'delete':
      $text = t('Are you sure you want to delete and re-index the text of all file attachments?');
      break;
    case 'clear-cache':
      $text = t('Are you sure you want to delete the cache of extracted text from file attachments?');
      break;
  }
  return confirm_form($form, $text, 'admin/config/search/apachesolr/attachments', NULL, t('Confirm'), t('Cancel'));
}

/**
 * Form submit handler for the index confirmation form
 *
 * @see apachesolr_attachments_confirm()
 */
function apachesolr_attachments_confirm_submit($form, &$form_state) {
  switch ($form_state['values']['operation']) {
    case 'delete':
      if (apachesolr_attachments_delete_index()) {
        drupal_set_message(t('File text has been deleted from the Apache Solr index. You must now <a href="!url">run cron</a> until all files have been re-indexed.', array('!url' => url('admin/reports/status/run-cron', array('query' => array('destination' => 'admin/config/search/apachesolr/index'))))));
      }
      else {
        if (module_exists('dblog')) {
          drupal_set_message(t('Could not delete file text from the Apache Solr index. Check <a href="!url">recent log messages</a>.', array('!url' => url('admin/reports/dblog'))));
        }
        else {
          drupal_set_message(t('Could not delete file text from the Apache Solr index.'));
        }
      }
      break;
    case 'reindex':
      apachesolr_clear_last_index('apachesolr_attachments');
      drupal_set_message(t('All file attachments will be re-indexed.'));
      break;
    case 'clear-cache':
      db_delete('apachesolr_attachments_files')->condition('removed', 0)->execute();
      drupal_set_message(t('The local cache of extracted text has been deleted.'));
      break;
  }
  $form_state['redirect'] = 'admin/config/search/apachesolr/attachments';
}

function apachesolr_attachments_delete_index() {
  try {
    $solr = apachesolr_get_solr();
    $solr->deleteByQuery("entity_type:file AND hash:" . apachesolr_site_hash());
    $solr->commit();
    apachesolr_index_set_last_updated(REQUEST_TIME);
    apachesolr_clear_last_index('apachesolr_attachments');
    return TRUE;
  }
  catch (Exception $e) {
    watchdog('Apache Solr Attachments', nl2br(check_plain($e->getMessage())), NULL, WATCHDOG_ERROR);
  }
  return FALSE;
}

/**
 * Indexing-related functions
 */

/**
 * Return all non-excluded file attachments for a particular node
 */
function apachesolr_attachments_get_indexable_files($node) {
  $files = array();

  $field_names = apachesolr_attachments_get_field_names();
  foreach ($field_names as $field_name) {
    if (!empty($node->$field_name)) {
      $field = $node->$field_name;
      list($lang, $values) = each($field);
      $files = array_merge($files, $values);
    }
  }

  $file_list = array();
  foreach ($files as $file) {
    if (apachesolr_attachments_allowed_mime($file['filemime'])) {
      $file_list[$file['fid']] = $file;
    }
  }
  return $file_list;
}

/**
 * Provides a default list of filename extensions to exclude from the index.
 *
 * @return
 *   An array of file extensions.
 */
function apachesolr_attachments_default_excluded() {
  $default = array('aif', 'art', 'avi', 'bmp', 'gif', 'ico', 'jpg', 'mov', 'mp3', 'mp4', 'mpg', 'oga', 'ogv', 'png', 'psd', 'ra', 'ram', 'rgb', 'tif');
  return $default;
}

/**
 * Checks if a file is of a MIME type that is to be excluded from the index.
 *
 * The MIME types of excluded files are built and cached each time the file
 * attachments settings form is saved.
 *
 * @param $filename
 *   A string, the MIME type of the file.
 *
 * @return
 *   Boolean, TRUE if the $filemime is allowed, otherwise FALSE.
 */
function apachesolr_attachments_allowed_mime($filemime) {

  $excluded = variable_get('apachesolr_attachment_excluded_mime', FALSE);
  if ($excluded === FALSE) {
    // Build the list of excluded MIME types.
    $excluded = array();
    $extensions = variable_get('apachesolr_attachment_excluded_extensions', FALSE);
    if ($extensions !== FALSE) {
      $extensions = explode(' ', $extensions);
    }
    else {
      $extensions = apachesolr_attachments_default_excluded();
    }
    foreach ($extensions as $ext) {
      $ext = trim($ext);
      if ($ext) {
        $mime = file_get_mimetype('dummy.' . $ext);
        $excluded[$mime] = 1;
      }
    }
    variable_set('apachesolr_attachment_excluded_mime', $excluded);
  }
  return empty($excluded[$filemime]);
}

/**
 * Return all fields that are of type 'file'
 */
function apachesolr_attachments_get_field_names() {
  $field_names = array();
  if (module_exists('file')) {
    $fields = field_info_fields();
    foreach ($fields as $field_name => $field) {
      if ($field['type'] == 'file') {
        $field_names[] = $field_name;
      }
    }
  }
  return $field_names;
}

/**
 * Parse the attachment getting just the raw text.
 *
 * @throws Exception
 */
function apachesolr_attachments_get_attachment_text($file) {
  $filepath = drupal_realpath($file['uri']);
  // Check that we have a valid filepath.
  if (!($filepath) || !is_file($filepath)) {
    return FALSE;
  }

  // No need to use java for plain text files.
  if ($file['filemime'] == 'text/plain' || $file['filemime'] == 'text/x-diff') {
    $text = file_get_contents($filepath);
    // TODO - try to detect encoding and convert to UTF-8.
    // Strip bad control characters.
    $text = iconv("UTF-8", "UTF-8//IGNORE", $text);
    $text = trim(apachesolr_clean_text($text));
    return $text;
  }

  $hash = hash('sha256', file_get_contents($filepath));
  if ($hash === FALSE) {
    return FALSE;
  }

  $cached = db_query("SELECT * FROM {apachesolr_attachments_files} WHERE fid = :fid", array(':fid' => $file['fid']))->fetchAssoc();

  if (!is_null($cached['body']) && ($cached['hash'] == $hash)) {
    // No need to re-extract.
    return $cached['body'];
  }

  if (variable_get('apachesolr_attachment_extract_using', 'tika') == 'tika') {
    $text = apachesolr_attachments_extract_using_tika($filepath);
  }
  else {
    // Extract using Solr.
    try {
      list($text, $metadata) = apachesolr_attachments_extract_using_solr($filepath);
    }
    catch (Exception $e) {
      // Exceptions from Solr may be transient, or indicate a problem with a specific file.
      watchdog('Apache Solr Attachments', "Exception occured sending %filepath to Solr\n!message", array('%filepath' => $file['uri'], '!message' => nl2br(check_plain($e->getMessage()))), WATCHDOG_ERROR);
      return '';
    }
  }
  // Strip bad control characters.
  $text = iconv("UTF-8", "UTF-8//IGNORE", $text);
  $text = trim(apachesolr_clean_text($text));
  // Save the extracted, cleaned text to the DB.
  db_update('apachesolr_attachments_files')->fields(array('hash' => $hash, 'body' => $text))->condition('fid', $file['fid'])->execute();

  return $text;
}

/**
 * For a file path, try to extract text using a local tika jar.
 *
 * @throws Exception
 */
function apachesolr_attachments_extract_using_tika($filepath) {
  $tika_path = realpath(variable_get('apachesolr_attachments_tika_path', ''));

  $tika = realpath($tika_path . '/' . variable_get('apachesolr_attachments_tika_jar', 'tika-0.3.jar'));
  if (!($tika) || !is_file($tika)) {
    throw new Exception(t('Invalid path or filename for tika application jar.'));
  }

  $java = variable_get('apachesolr_attachments_java', 'java');
  // By default force UTF-8 output.
  $java_opts = ' ' . variable_get('apachesolr_attachments_java_opts',  '-Dfile.encoding=UTF8');
  $cmd = escapeshellcmd($java . $java_opts) . ' -cp ' . escapeshellarg($tika_path) . ' -jar ' . escapeshellarg($tika) . ' -t ' . escapeshellarg($filepath);

  // Add a work-around for a MAMP bug + java 1.5.
  if (strpos(ini_get('extension_dir'), 'MAMP/')) {
    $cmd = 'export DYLD_LIBRARY_PATH=""; ' . $cmd;
  }

  return shell_exec($cmd);
}

/**
 * For a file path, try to extract text using Solr 1.4.
 *
 * @throws Exception
 */
function apachesolr_attachments_extract_using_solr($filepath) {
  // Extract using Solr.
  // We allow Solr to throw exceptions - they will be caught
  // by apachesolr.module.
  $solr = apachesolr_get_solr();
  $filename = basename($filepath);
  $params = array(
    'resource.name' => $filename,
    'extractFormat' => 'text', // Matches the -t command for the tika CLI app.
  );
  // Construct a multi-part form-data POST body in $data.
  $boundary = '--' . md5(uniqid(REQUEST_TIME));
  $data = "--{$boundary}\r\n";
  // The 'filename' used here becomes the property name in the response.
  $data .= 'Content-Disposition: form-data; name="file"; filename="extracted"';
  $data .= "\r\nContent-Type: application/octet-stream\r\n\r\n";
  $data .= file_get_contents($filepath);
  $data .= "\r\n--{$boundary}--\r\n";
  $headers = array('Content-Type' => 'multipart/form-data; boundary=' . $boundary);
  $response = $solr->makeServletRequest(EXTRACTING_SERVLET, $params, 'POST', $headers, $data);
  return array($response->extracted, $response->extracted_metadata);
}
