import React, { useState } from 'react'
import { Upload, Settings, Film, Download, AlertCircle } from 'lucide-react';
import { Alert, AlertDescription } from './components/ui/alert';
/* import reactLogo from './assets/react.svg'
import viteLogo from '/vite.svg'
import './App.css' */

const VideoCodecApp = () => {
  const [activeTab, setActiveTab] = useState('compress');
  const [loading, setLoading] = useState(false);
  const [notification, setNotification] = useState(null);
  const [fileDetails, setFileDetails] = useState(null);
  const [formData, setFormData] = useState({
    width: '',
    height: '',
    num_frames: ''
  });

  const handleFileChange = (e) => {
    const file = e.target.files[0];
    if (file) {
      setFileDetails({
        file,
        name: file.name,
        size: (file.size / 1024).toFixed(2) + ' KB'
      });
      // Clear any existing notifications
      setNotification(null);
    }
  };

  const handleInputChange = (e) => {
    const { name, value } = e.target;
    setFormData(prev => ({
      ...prev,
      [name]: value
    }));
  };

  const processVideo = async (e) => {
    e.preventDefault();
    if (!fileDetails?.file) {
      setNotification({
        type: "error",
        message: 'Please select a file first'
      });
      return;
    }

    setLoading(true);
    setNotification(null);

    const submitData = new FormData();
    submitData.append('file', fileDetails.file)
    submitData.append('width', fileDetails.width)
    submitData.append('height', formData.height);
    if (activeTab === 'decompress') {
      submitData.append('num_frames', formData.num_frames);
    }

    try {
      const response = await fetch(`/api/${activeTab}`, {
        method: "POST",
        body: submitData
      });


      const responseText = await response.text()
      //const data = await response.json();
      console.log('Raw response: ', responseText);

      try {
        const data = JSON.parse(responseText)

        if (data.success) {
          // show success worth stats
          let successMessage = activeTab === 'compress'
          ? `Compression complete! Ratio; ${data.data.compression_ratio}`
          : `Decompression complete! ${data.data.frame_count} frames processed`;

          setNotification({
            type: 'success',
            message: successMessage,
            data: data.data
          });

          // Trigger download
          window.location.href = data.data.download_url;
        } else {
          throw new Error(data.error || 'Processing failed');
        }
      } catch (jsonError) {
        console.error("JSON parsing Error: ", jsonError)
        setNotification({
          type: 'error',
          message: "Failed to parse server response"
        })
      }
    } catch (error) {
      setNotification({
        type: 'error',
        message: error.message || 'An unexpected error occured'
      });
    } finally {
      setLoading(false);
    }
  };

  const resetForm = () => {
    setFileDetails(null);
    setNotification(null);
    setFormData({
      width: '',
      height: '',
      num_frames: ''
    });
    // reset failed
    const fileInput = document.getElementById('file-upload');
    if (fileInput) {
      fileInput.value = '';
    }
  };

  return (
    <div className="min-h-screen bg-gray-50">
      <header className="text-white bg-blue-600 shadow-lg">
        <div className="container px-4 py-6 mx-auto">
          <h1 className="text-3xl font-bold">Video Codec Application</h1>
          <p className="mt-2 text-blue-100">Compress and decompress your videos efficiently</p>
        </div>
      </header>

      <main className="container px-4 py-8 mx-auto">
        <div className="overflow-hidden bg-white rounded-lg shadow-md">
          {/* Tab Navigation */}
          <div className="flex border-b">
            <button
              onClick={() => {
                setActiveTab('compress');
                resetForm();
              }}
              className={`flex items-center px-6 py-4 focus:outline-none ${
                activeTab === 'compress'
                  ? 'bg-blue-50 border-b-2 border-blue-600 text-blue-600'
                  : 'text-gray-600 hover:bg-gray-50'
              }`}
            >
              <Settings className="w-5 h-5 mr-2" />
              Compress Video
            </button>
            <button
              onClick={() => {
                setActiveTab('decompress');
                resetForm();
              }}
              className={`flex items-center px-6 py-4 focus:outline-none ${
                activeTab === 'decompress'
                  ? 'bg-blue-50 border-b-2 border-blue-600 text-blue-600'
                  : 'text-gray-600 hover:bg-gray-50'
              }`}
            >
              <Film className="w-5 h-5 mr-2" />
              Decompress Video
            </button>
          </div>

          {/* Main Content */}
          <div className="p-6">
            {notification && (
              <Alert className="mb-6" variant={notification.type === 'error' ? 'destructive' : 'default'}>
                <AlertCircle className="w-4 h-4 mr-2" />
                <AlertDescription>
                  {notification.message}
                  {notification.data && notification.data.compression_ratio && (
                    <div className="mt-2 text-sm">
                      <div>Original size: {(notification.data.original_size / 1024).toFixed(2)} KB</div>
                      <div>Compressed size: {(notification.data.compressed_size / 1024).toFixed(2)} KB</div>
                    </div>
                  )}
                </AlertDescription>
              </Alert>
            )}

            <form onSubmit={processVideo} className="space-y-6">
              {/* File Upload Section */}
              <div className="space-y-4">
                <label className="block text-sm font-medium text-gray-700">
                  {activeTab === 'compress' ? 'Raw Video File (RGB24)' : 'Compressed Video File (BIN)'}
                </label>
                <div className="p-6 text-center border-2 border-gray-300 border-dashed rounded-lg">
                  <input
                    type="file"
                    onChange={handleFileChange}
                    className="hidden"
                    id="file-upload"
                    accept={activeTab === 'compress' ? '.rgb24' : '.bin'}
                  />
                  <label
                    htmlFor="file-upload"
                    className="flex flex-col items-center justify-center cursor-pointer"
                  >
                    <Upload className="w-12 h-12 text-gray-400" />
                    <span className="mt-2 text-sm text-gray-600">
                      Click to upload or drag and drop
                    </span>
                    <span className="mt-1 text-xs text-gray-500">
                      Maximum file size: 100MB
                    </span>
                  </label>
                  {fileDetails && (
                    <div className="mt-4 text-sm text-gray-600">
                      Selected: {fileDetails.name} ({fileDetails.size})
                    </div>
                  )}
                </div>
              </div>

              {/* Video Settings */}
              <div className="grid grid-cols-1 gap-6 md:grid-cols-2">
                <div>
                  <label className="block text-sm font-medium text-gray-700">Width</label>
                  <input
                    type="number"
                    name="width"
                    value={formData.width}
                    onChange={handleInputChange}
                    required
                    min="1"
                    className="block w-full mt-1 border-gray-300 rounded-md shadow-sm focus:border-blue-500 focus:ring-blue-500"
                    placeholder="e.g., 1920"
                  />
                </div>
                <div>
                  <label className="block text-sm font-medium text-gray-700">Height</label>
                  <input
                    type="number"
                    name="height"
                    value={formData.height}
                    onChange={handleInputChange}
                    required
                    min="1"
                    className="block w-full mt-1 border-gray-300 rounded-md shadow-sm focus:border-blue-500 focus:ring-blue-500"
                    placeholder="e.g., 1080"
                  />
                </div>
                {activeTab === 'decompress' && (
                  <div className="md:col-span-2">
                    <label className="block text-sm font-medium text-gray-700">
                      Number of Frames
                    </label>
                    <input
                      type="number"
                      name="num_frames"
                      value={formData.num_frames}
                      onChange={handleInputChange}
                      required
                      min="1"
                      className="block w-full mt-1 border-gray-300 rounded-md shadow-sm focus:border-blue-500 focus:ring-blue-500"
                      placeholder="e.g., 120"
                    />
                  </div>
                )}
              </div>

              {/* Action Buttons */}
              <div className="flex justify-end space-x-4">
                <button
                  type="button"
                  onClick={resetForm}
                  className="px-4 py-2 text-gray-700 border border-gray-300 rounded-md hover:bg-gray-50 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-blue-500"
                >
                  Clear
                </button>
                <button
                  type="submit"
                  disabled={loading || !fileDetails}
                  className="flex items-center px-4 py-2 text-white bg-blue-600 rounded-md hover:bg-blue-700 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-blue-500 disabled:opacity-50 disabled:cursor-not-allowed"
                >
                  {loading ? (
                    <span className="flex items-center">
                      <svg className="w-5 h-5 mr-3 -ml-1 text-white animate-spin" xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24">
                        <circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4"></circle>
                        <path className="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z"></path>
                      </svg>
                      Processing...
                    </span>
                  ) : (
                    <>
                      <span>{activeTab === 'compress' ? 'Compress' : 'Decompress'}</span>
                      <Download className="w-4 h-4 ml-2" />
                    </>
                  )}
                </button>
              </div>
            </form>
          </div>
        </div>
      </main>

      <footer className="mt-12 text-white bg-gray-800">
        <div className="container px-4 py-6 mx-auto text-center">
          <p>&copy; 2025 Video Codec Application. All rights reserved.</p>
        </div>
      </footer>
    </div>
  );
};

export default VideoCodecApp;
