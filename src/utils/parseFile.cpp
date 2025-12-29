/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parseFile.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/01 01:01:11 by taung             #+#    #+#             */
/*   Updated: 2025/12/30 03:06:11 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "../../include/utils.h"

// bool	parseFile(std::string body, std::string contentType, std::string &fileName, std::string &fileContent) {
// 	std::cout << "parseFile POST" << body << contentType << fileName << fileContent << std::endl;
// 	// Extract boundary from Content-Type
// 	std::string boundaryKey = "boundary=";
// 	size_t bpos = contentType.find(boundaryKey);
// 	if (bpos == std::string::npos)
// 		return false;

// 	std::string boundary = "--" + contentType.substr(bpos + boundaryKey.length());
// 	std::string endBoundary = boundary + "--";

// 	// 1) Find first boundary inside the body
// 	size_t partStart = body.find(boundary);
// 	if (partStart == std::string::npos)
// 		return false;

// 	partStart += boundary.length() + 2; // skip boundary + CRLF

// 	// 2) Find Content-Disposition header
// 	size_t cdPos = body.find("Content-Disposition:", partStart);
// 	if (cdPos == std::string::npos)
// 		return false;

// 	size_t cdEnd = body.find("\r\n", cdPos);
// 	if (cdEnd == std::string::npos)
// 		return false;

// 	std::string cdLine = body.substr(cdPos, cdEnd - cdPos);

// 	// Extract filename
// 	std::string filenameKey = "filename=\"";
// 	size_t fpos = cdLine.find(filenameKey);
// 	if (fpos == std::string::npos)
// 		return false;

// 	fpos += filenameKey.length();
// 	size_t fend = cdLine.find("\"", fpos);
// 	if (fend == std::string::npos)
// 		return false;

// 	fileName = cdLine.substr(fpos, fend - fpos);

// 	// 3) Find Content-Type header (optional but normal)
// 	size_t ctPos = body.find("Content-Type:", cdEnd);
// 	if (ctPos == std::string::npos)
// 		return false;

// 	size_t ctEnd = body.find("\r\n", ctPos);
// 	if (ctEnd == std::string::npos)
// 		return false;

// 	// 4) Skip double CRLF to reach file content
// 	size_t dataStart = body.find("\r\n\r\n", ctEnd);
// 	if (dataStart == std::string::npos)
// 		return false;

// 	dataStart += 4; // skip "\r\n\r\n"

// 	// 5) Find the boundary that marks end of file content
// 	size_t dataEnd = body.find(boundary, dataStart);
// 	if (dataEnd == std::string::npos)
// 		return false;

// 	// Remove last CRLF before boundary
// 	if (dataEnd >= 2 && body.substr(dataEnd - 2, 2) == "\r\n")
// 		dataEnd -= 2;

// 	// 6) Extract the actual file content
// 	fileContent = body.substr(dataStart, dataEnd - dataStart);

// 	return true;
// }

bool	parseFile(std::string body, std::string contentType, std::vector<std::string> &fileNames, std::vector<std::string> &fileContents) {

	// -----------------------
    // 1. Extract boundary
    // -----------------------
    std::string boundaryKey = "boundary=";
    size_t bpos = contentType.find(boundaryKey);
    if (bpos == std::string::npos)
        return false;

    std::string boundary = "--" + contentType.substr(bpos + boundaryKey.length());
    std::string endBoundary = boundary + "--";

    // -----------------------
    // 2. Loop through all parts
    // -----------------------
    size_t pos = 0;

    while (true) {
        // Find next boundary
        size_t partStart = body.find(boundary, pos);
        if (partStart == std::string::npos)
            break;  // no more parts

        partStart += boundary.length();
        if (body.compare(partStart, 2, "\r\n") == 0)
            partStart += 2; // skip CRLF

        // Reached final boundary?
        if (body.compare(partStart, 2, "--") == 0)
            break;

        // -----------------------------------------
        // 3. Parse headers inside this part
        // -----------------------------------------
        size_t cdPos = body.find("Content-Disposition:", partStart);
        if (cdPos == std::string::npos)
            return false;

        size_t cdEnd = body.find("\r\n", cdPos);
        if (cdEnd == std::string::npos)
            return false;

        std::string cdLine = body.substr(cdPos, cdEnd - cdPos);

        // Extract filename="..."
        std::string filenameKey = "filename=\"";
        size_t fpos = cdLine.find(filenameKey);
        if (fpos == std::string::npos)
            return false;

        fpos += filenameKey.length();
        size_t fend = cdLine.find("\"", fpos);
        if (fend == std::string::npos)
            return false;

        std::string fileName = cdLine.substr(fpos, fend - fpos);

        // -----------------------------------------
        // 4. Find Content-Type (optional)
        // -----------------------------------------
        size_t ctPos = body.find("Content-Type:", cdEnd);
        if (ctPos == std::string::npos)
            return false;

        size_t ctEnd = body.find("\r\n", ctPos);
        if (ctEnd == std::string::npos)
            return false;

        // -----------------------------------------
        // 5. Skip \r\n\r\n to reach file data
        // -----------------------------------------
        size_t dataStart = body.find("\r\n\r\n", ctEnd);
        if (dataStart == std::string::npos)
            return false;

        dataStart += 4;

        // -----------------------------------------
        // 6. Find next boundary = end of this file
        // -----------------------------------------
        size_t dataEnd = body.find(boundary, dataStart);
        if (dataEnd == std::string::npos)
            return false;

        // Remove trailing CRLF before boundary
        if (dataEnd >= 2 && body.compare(dataEnd - 2, 2, "\r\n") == 0)
            dataEnd -= 2;

        std::string fileContent = body.substr(dataStart, dataEnd - dataStart);

        // -----------------------------------------
        // 7. Store results
        // -----------------------------------------
        fileNames.push_back(fileName);
        fileContents.push_back(fileContent);

        // Move cursor to continue parsing next part
        pos = dataEnd;
    }

    return !fileNames.empty(); // return true if at least one file parsed
}

bool	parseFile(std::string urlPath, std::string rootPath, std::string &filePath) {
	// std::cout << "parseFile DELETE: " << urlPath << "\n" << filePath << "\n" << rootPath << std::endl;

	// check if there is a ".." in the urlPath for safty
	if (urlPath.find("..") == std::string::npos) {
		filePath = rootPath + urlPath;
		return (true);
	}
	return (false);
}
